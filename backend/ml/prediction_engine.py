"""
Prediction Engine Coordinator for the AI Inventory Management Backend.
Coordinates feature engineering, forecasting, classification, EOQ, priority scoring,
explainability, model caching, and cold-start rules.
"""

import pandas as pd
import numpy as np
from datetime import datetime
from typing import List, Dict, Any, Tuple
from sklearn.cluster import KMeans

from backend.config import settings
from backend.models.entities import MLResult
from backend.ml.feature_engineering import extract_features_for_product
from backend.ml.forecasting import forecast_demand
from backend.ml.classifier import train_classifier, predict_health
from backend.ml.eoq import calculate_eoq
from backend.ml.priority import calculate_priority_score
from backend.ml.explainability import generate_explanation
from backend.ml.model_manager import should_retrain, save_model, load_model
from backend.utils.helpers import compute_data_hash
from backend.utils.logger import get_logger

logger = get_logger("prediction_engine")

# Feature columns used for Random Forest classification
CLASSIFIER_FEATURES = [
    "sales_volume", "lag_1", "lag_2", "lag_3", "lag_7",
    "rolling_mean_7", "rolling_std_7", "rolling_sum_7",
    "rolling_mean_30", "rolling_std_30", "rolling_sum_30",
    "ema_7", "ema_30", "growth_rate", "current_stock",
    "unit_cost", "reorder_point", "lead_time", "order_cost",
    "holding_cost", "average_usage", "demand_trend"
]

def run_ml_pipeline(
    sales_history_df: pd.DataFrame,
    products_list: List[Dict[str, Any]],
    lead_time_days: int = 7,
    ordering_cost: float = 20.0,
    holding_cost_rate: float = 0.25,
    forecast_horizon: int = 7,
    use_prophet: bool = True
) -> List[MLResult]:
    """
    Executes the entire AI inventory prediction, forecasting, classification,
    and prioritization pipeline. Handles cold starts and persists trained models.
    
    Args:
        sales_history_df: DataFrame with columns: sku, name, category, sales_volume, date
        products_list: List of dicts representing each product's metadata (sku, current_stock, unit_cost, reorder_point, id)
        lead_time_days: Supplier lead time.
        ordering_cost: Ordering setup cost.
        holding_cost_rate: Holding cost percentage of unit cost.
        forecast_horizon: Number of days to forecast.
        
    Returns:
        List[MLResult]: Pipeline output results.
    """
    if sales_history_df.empty or not products_list:
        logger.warning("Sales history or product list is empty. Returning empty predictions.")
        return []
        
    logger.info(f"Starting ML pipeline for {len(products_list)} products...")
    
    # ── K-Means Clustering for Demand Labels ──
    avg_sales_by_sku = sales_history_df.groupby("sku")["sales_volume"].mean().to_dict()
    skus_for_clustering = list(avg_sales_by_sku.keys())
    
    if len(skus_for_clustering) > 0:
        X_kmeans = np.array([avg_sales_by_sku[sku] for sku in skus_for_clustering]).reshape(-1, 1)
        n_clusters = min(3, len(X_kmeans))
        kmeans = KMeans(n_clusters=n_clusters, random_state=42, n_init=10)
        cluster_labels = kmeans.fit_predict(X_kmeans)
        
        # Sort clusters by average sales volume (lowest to highest)
        cluster_centers = kmeans.cluster_centers_.flatten()
        sorted_cluster_indices = np.argsort(cluster_centers)
        
        cluster_demand_map = {}
        if n_clusters == 3:
            cluster_demand_map[sorted_cluster_indices[0]] = "Low Demand"
            cluster_demand_map[sorted_cluster_indices[1]] = "Medium Demand"
            cluster_demand_map[sorted_cluster_indices[2]] = "High Demand"
        elif n_clusters == 2:
            cluster_demand_map[sorted_cluster_indices[0]] = "Low Demand"
            cluster_demand_map[sorted_cluster_indices[1]] = "High Demand"
        else:
            cluster_demand_map[sorted_cluster_indices[0]] = "Medium Demand"
            
        demand_labels_by_sku = {
            skus_for_clustering[i]: cluster_demand_map[cluster_labels[i]]
            for i in range(len(skus_for_clustering))
        }
    else:
        demand_labels_by_sku = {}
        
    # Map products by SKU for easy lookup
    products_map = {p["sku"]: p for p in products_list}
    
    # ── 1. Check Cold Start and Extract Features ──
    # Separate cold start products from products with enough history for ML training
    ml_products_features = []
    cold_start_skus = set()
    features_by_sku = {}
    
    for sku, sku_df in sales_history_df.groupby("sku"):
        if sku not in products_map:
            continue
            
        p_info = products_map[sku]
        
        # Determine history length:
        # Require at least 4 unique dates or a range of 28 days
        unique_dates = sku_df["date"].nunique()
        date_span = 0
        if len(sku_df) > 1:
            try:
                date_span = (pd.to_datetime(sku_df["date"].max()) - pd.to_datetime(sku_df["date"].min())).days
            except Exception:
                pass
                
        is_cold_start = unique_dates < 4 and date_span < 28
        
        if is_cold_start:
            cold_start_skus.add(sku)
            logger.info(f"Product SKU={sku} is in Cold Start (history={unique_dates} points, span={date_span} days).")
            continue
            
        try:
            # Extract features for all historical time-series points
            feat_df = extract_features_for_product(
                sku_df,
                p_info,
                lead_time_days=lead_time_days,
                ordering_cost=ordering_cost,
                holding_cost_rate=holding_cost_rate
            )
            if not feat_df.empty:
                ml_products_features.append(feat_df)
                features_by_sku[sku] = feat_df
        except Exception as e:
            logger.error(f"Failed feature extraction for SKU={sku}: {e}", exc_info=True)
            cold_start_skus.add(sku)
            
    # ── 2. Train status classifier (Random Forest) if ML products exist ──
    clf = None
    scaler = None
    
    if ml_products_features:
        # Combine all features into a single training dataframe
        combined_features = pd.concat(ml_products_features, ignore_index=True)
        data_hash = compute_data_hash(combined_features)
        
        try:
            if should_retrain(data_hash):
                clf, scaler, acc = train_classifier(combined_features, CLASSIFIER_FEATURES)
                save_model(clf, scaler, CLASSIFIER_FEATURES, data_hash)
            else:
                clf, scaler, _ = load_model()
        except Exception as e:
            logger.error(f"Error training/loading classifier model: {e}", exc_info=True)
            # Rollback to heuristic fallback inside predict_health
            clf, scaler = None, None
            
    # ── 3. Run Forecasting & Generate Final Outputs ──
    results = []
    
    for p in products_list:
        sku = p["sku"]
        product_id = p.get("id", 0)
        current_stock = int(p.get("current_stock", 0))
        reorder_point = int(p.get("reorder_point", 10))
        unit_cost = float(p.get("unit_cost", 0.0))
        
        # Default fallback values for cold-start or error scenarios
        avg_usage = 0.0
        sku_df = sales_history_df[sales_history_df["sku"] == sku]
        if not sku_df.empty:
            avg_usage = max(0.0, float(sku_df["sales_volume"].mean()))
            
        if sku in cold_start_skus or sku not in features_by_sku:
            # ── COLD START RULE-BASED PATH ──
            forecast_7d = avg_usage * forecast_horizon
            trend = 0.0
            
            # Simple rule-based classification
            safety_stock = avg_usage * 2.0
            overstock_thresh = max(reorder_point * 3.0, avg_usage * 30.0)
            
            if current_stock <= safety_stock:
                status = "Critical"
                priority = "Critical"
            elif current_stock <= reorder_point:
                status = "Low"
                priority = "Reorder Soon"
            elif current_stock > overstock_thresh:
                status = "Overstock"
                priority = "Safe"
            else:
                status = "Healthy"
                priority = "Safe"
                
            # Force Critical priority if stock is very low
            if current_stock <= safety_stock or current_stock <= (reorder_point * 0.2):
                priority = "Critical"
                
            confidence = 50.0  # Default confidence for cold-start heuristics
            eoq = calculate_eoq(avg_usage, ordering_cost, holding_cost_rate, unit_cost)
            
            # Generate flat forecast points
            forecast_points = []
            today = datetime.now()
            for i in range(forecast_horizon):
                forecast_points.append({
                    "ds": (today + pd.Timedelta(days=i+1)).strftime("%Y-%m-%d"),
                    "yhat": avg_usage,
                    "yhat_lower": avg_usage * 0.8,
                    "yhat_upper": avg_usage * 1.2
                })
                
            explanation = (
                f"Product '{p['name']}' (SKU: {sku}) is in cold-start phase (fewer than 4 weeks of history). "
                f"Rule-based estimation is active. Current stock of {current_stock} units is classified as '{status}'."
            )
            
            res = MLResult(
                product_id=product_id,
                sku=sku,
                demand_label=demand_labels_by_sku.get(sku, "Medium Demand"),
                stock_status=status,
                confidence=confidence,
                forecast_7d=forecast_7d,
                trend=trend,
                eoq=eoq,
                priority=priority,
                explanations=explanation,
                forecast_points=forecast_points
            )
            results.append(res)
            
        else:
            # ── ML TRAINING & PREDICTION PATH ──
            try:
                # 1. Forecasting
                forecast_7d, trend, fcst_pts, model_used = forecast_demand(
                    sku_df, forecast_horizon, use_prophet=use_prophet
                )
                
                # 2. Classification Feature Setup (use the last row representing the current state)
                feat_df = features_by_sku[sku]
                current_state_row = feat_df.iloc[-1].to_dict()
                
                # Overwrite current stock in feature row in case it has changed since sales logs
                current_state_row["current_stock"] = current_stock
                current_state_row["reorder_point"] = reorder_point
                current_state_row["unit_cost"] = unit_cost
                current_state_row["holding_cost"] = unit_cost * holding_cost_rate
                
                # Predict Status
                if clf and scaler:
                    status, confidence = predict_health(clf, scaler, current_state_row, CLASSIFIER_FEATURES)
                else:
                    # In case of training failures, fall back to heuristic helper
                    from backend.ml.classifier import determine_heuristic_target
                    status = determine_heuristic_target(current_state_row)
                    confidence = 50.0
                    
                # 3. Calculate EOQ
                eoq = calculate_eoq(avg_usage, ordering_cost, holding_cost_rate, unit_cost)
                
                # 4. Calculate Priority score
                priority_score = calculate_priority_score(
                    forecast_7d=forecast_7d,
                    current_stock=current_stock,
                    reorder_point=reorder_point,
                    lead_time=lead_time_days,
                    trend=trend,
                    average_usage=avg_usage,
                    stock_status=status
                )
                
                priority_label = "Safe"
                # Force Critical priority if stock is very low (<= 20% of reorder point)
                if status == "Critical" or current_stock <= (reorder_point * 0.2):
                    priority_label = "Critical"
                elif priority_score >= 75.0:
                    priority_label = "Critical"
                elif priority_score >= 50.0:
                    priority_label = "Reorder Soon"
                    
                # 5. Determine demand label using K-Means clustering
                demand_label = demand_labels_by_sku.get(sku, "Medium Demand")
                    
                # 6. Generate Explanation
                explanation = generate_explanation(
                    sku=sku,
                    name=p["name"],
                    current_stock=current_stock,
                    reorder_point=reorder_point,
                    forecast_7d=forecast_7d,
                    lead_time=lead_time_days,
                    trend=trend,
                    eoq=eoq,
                    priority=priority_label,
                    stock_status=status
                )
                
                res = MLResult(
                    product_id=product_id,
                    sku=sku,
                    demand_label=demand_label,
                    stock_status=status,
                    confidence=confidence,
                    forecast_7d=forecast_7d,
                    trend=trend,
                    eoq=eoq,
                    priority=priority_label,
                    explanations=explanation,
                    forecast_points=fcst_pts
                )
                results.append(res)
                
            except Exception as e:
                logger.error(f"Error executing predictions for SKU={sku}: {e}", exc_info=True)
                # Fallback to cold-start rules if prediction fails
                cold_start_skus.add(sku)
                
    logger.info(f"ML Pipeline execution finished. Generated {len(results)} predictions.")
    return results
