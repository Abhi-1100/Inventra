"""
Prediction Service for the AI Inventory Management Backend.
Coordinates pipeline execution, handles DB loading, evaluates forecast accuracy,
and saves pipeline run reports.
"""

import json
import pandas as pd
from datetime import datetime
from typing import List, Dict, Any, Optional

from backend.database.db_manager import DBManager
from backend.models.entities import MLResult
from backend.ml.prediction_engine import run_ml_pipeline
from backend.ml.evaluation import evaluate_forecast, evaluate_classifier
from backend.utils.logger import get_logger

logger = get_logger("prediction_service")

class PredictionService:
    """
    Executes and evaluates ML predictions by loading history from DB.
    """
    def __init__(self, db_manager: DBManager):
        self.db = db_manager

    def get_prediction_history(self, limit: int = 50) -> List[Dict[str, Any]]:
        """
        Retrieves ML pipeline historical run records from the database.
        """
        return self.db.get_latest_pipeline_results(limit=limit)

    def run_predictions(
        self,
        lead_time_days: int = 7,
        ordering_cost: float = 20.0,
        holding_cost_rate: float = 0.25,
        forecast_horizon: int = 7
    ) -> List[MLResult]:
        """
        Loads product and sales history from database and triggers the ML pipeline.
        Saves run results in pipeline_results.
        """
        try:
            # 1. Load active products
            all_products = self.db.list_products()
            deactivated_json = self.db.get_setting("deactivated_skus", "[]")
            deactivated_list = json.loads(deactivated_json)
            
            active_products = [p for p in all_products if p.sku not in deactivated_list]
            if not active_products:
                logger.warning("No active products to run predictions for.")
                return []
                
            products_list = []
            for p in active_products:
                products_list.append({
                    "id": p.id,
                    "sku": p.sku,
                    "name": p.name,
                    "category": p.category,
                    "current_stock": p.current_stock,
                    "unit_cost": p.unit_cost,
                    "reorder_point": p.reorder_point
                })
                
            # 2. Load historical sales from daily_entries
            sales_query = """
            SELECT p.sku, p.name, p.category, de.units_sold as sales_volume, 
                   de.entry_date as date, p.id as product_id
            FROM daily_entries de
            JOIN products p ON de.product_id = p.id
            ORDER BY de.entry_date ASC;
            """
            
            with self.db.connection() as conn:
                cursor = conn.cursor()
                cursor.execute(sales_query)
                rows = cursor.fetchall()
                
            if not rows:
                logger.warning("No sales history records in daily_entries. Pipeline cannot run.")
                return []
                
            sales_df = pd.DataFrame([dict(r) for r in rows])
            
            # 3. Execute ML pipeline
            results = run_ml_pipeline(
                sales_history_df=sales_df,
                products_list=products_list,
                lead_time_days=lead_time_days,
                ordering_cost=ordering_cost,
                holding_cost_rate=holding_cost_rate,
                forecast_horizon=forecast_horizon
            )
            
            # 4. Save results to pipeline_results
            if results:
                serializable_results = []
                for r in results:
                    serializable_results.append({
                        "product_id": r.product_id,
                        "sku": r.sku,
                        "demand_label": r.demand_label,
                        "stock_status": r.stock_status,
                        "confidence": r.confidence,
                        "forecast_7d": r.forecast_7d,
                        "trend": r.trend,
                        "eoq": r.eoq,
                        "priority": r.priority,
                        "explanations": r.explanations,
                        "forecast_points": r.forecast_points
                    })
                
                results_json = json.dumps(serializable_results)
                run_at = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                self.db.save_pipeline_result(run_at, True, results_json)
                
            return results
            
        except Exception as e:
            logger.error(f"Error running prediction service pipeline: {e}", exc_info=True)
            # Save failed pipeline status
            try:
                run_at = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                self.db.save_pipeline_result(run_at, False, json.dumps({"error": str(e)}))
            except Exception as db_err:
                logger.error(f"Failed to record pipeline failure: {db_err}")
            raise

    def run_pipeline_with_dataframe(
        self,
        sales_df: pd.DataFrame,
        lead_time_days: int = 7,
        ordering_cost: float = 20.0,
        holding_cost_rate: float = 0.25,
        forecast_horizon: int = 7
    ) -> List[MLResult]:
        """
        Executes prediction pipeline on an external DataFrame (e.g. from C++ CSV bridge).
        """
        try:
            # Construct product list from unique SKUs in DataFrame
            products_list = []
            grouped = sales_df.groupby("sku")
            for sku, group in grouped:
                # Grab details from the last row
                last_row = group.iloc[-1]
                
                raw_id = last_row.get("product_id", 0)
                try:
                    product_id = int(raw_id)
                except ValueError:
                    import re
                    num_part = re.sub(r"\D", "", str(raw_id))
                    product_id = int(num_part) if num_part else 0
                    
                products_list.append({
                    "id": product_id,
                    "sku": sku,
                    "name": str(last_row.get("name", sku)),
                    "category": str(last_row.get("category", "")),
                    "current_stock": int(last_row.get("current_stock", 0)),
                    "unit_cost": float(last_row.get("unit_cost", 0.0)),
                    "reorder_point": int(last_row.get("reorder_point", 10))
                })
                
            return run_ml_pipeline(
                sales_history_df=sales_df,
                products_list=products_list,
                lead_time_days=lead_time_days,
                ordering_cost=ordering_cost,
                holding_cost_rate=holding_cost_rate,
                forecast_horizon=forecast_horizon
            )
        except Exception as e:
            logger.error(f"Error executing pipeline with DataFrame: {e}", exc_info=True)
            raise
