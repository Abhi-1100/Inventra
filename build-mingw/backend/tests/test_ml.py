"""
Unit Tests for Machine Learning Pipeline Components.
Tests feature engineering, forecasting fallbacks, classification models, EOQ, priority scores,
explainability templates, model managers, and evaluation metrics.
"""

import os
import pytest
import numpy as np
import pandas as pd
from datetime import datetime

from backend.ml.feature_engineering import extract_features_for_product
from backend.ml.forecasting import forecast_demand
from backend.ml.classifier import train_classifier, predict_health, determine_heuristic_target
from backend.ml.eoq import calculate_eoq
from backend.ml.priority import calculate_priority_score
from backend.ml.explainability import generate_explanation
from backend.ml.evaluation import evaluate_forecast, evaluate_classifier
from backend.ml.model_manager import should_retrain, save_model, load_model
from backend.ml.prediction_engine import run_ml_pipeline

def test_feature_engineering_no_nans():
    """Verifies that feature engineering output is structured and contains no NaNs."""
    history = pd.DataFrame({
        "date": pd.date_range(start="2026-06-01", periods=15, freq="D"),
        "sales_volume": [2, 3, 0, 5, 4, 1, 3, 2, 4, 6, 8, 4, 3, 5, 2]
    })
    
    product_info = {"current_stock": 25, "unit_cost": 10.0, "reorder_point": 10}
    
    features = extract_features_for_product(history, product_info)
    
    assert not features.empty
    assert "lag_1" in features.columns
    assert "rolling_mean_7" in features.columns
    assert "average_usage" in features.columns
    assert "demand_trend" in features.columns
    
    # Assert absolutely no NaN values remain
    assert features.isna().sum().sum() == 0

def test_forecasting_fallbacks():
    """Tests Prophet, LinearRegression, and Moving Average fallback stages."""
    # Case 1: Minimal history (Moving Average fallback)
    short_df = pd.DataFrame({
        "date": ["2026-07-01", "2026-07-02", "2026-07-03"],
        "sales_volume": [10, 20, 30]
    })
    
    total, trend, points, model = forecast_demand(short_df, horizon=5, start_date=datetime(2026, 7, 3))
    assert model in ("Linear Regression", "Moving Average")
    assert len(points) == 5
    
    # Case 2: Intermediate history (fits Linear Regression)
    med_df = pd.DataFrame({
        "date": pd.date_range(start="2026-07-01", periods=8, freq="D"),
        "sales_volume": [2, 4, 6, 8, 10, 12, 14, 16]
    })
    total, trend, points, model = forecast_demand(med_df, horizon=3, start_date=datetime(2026, 7, 8))
    assert model in ("Prophet", "Linear Regression")
    assert len(points) == 3
    assert total > 0

def test_forecasting_prophet_success():
    """Tests that Prophet forecasting runs and returns predictions when enough history exists."""
    df = pd.DataFrame({
        "date": pd.date_range(start="2026-06-01", periods=15, freq="D"),
        "sales_volume": [5, 4, 6, 5, 7, 6, 8, 7, 9, 8, 10, 9, 11, 10, 12]
    })
    total, trend, points, model = forecast_demand(df, horizon=7, start_date=datetime(2026, 6, 15))
    assert model == "Prophet"
    assert len(points) == 7
    assert total > 0

def test_classifier_training_and_prediction():
    """Tests Random Forest Classifier fitting and health predictions."""
    # Mock feature dataset
    data = {
        "sales_volume": [10, 2, 15, 0, 12],
        "lag_1": [9, 3, 14, 1, 10],
        "lag_2": [8, 4, 13, 2, 9],
        "lag_3": [7, 5, 12, 3, 8],
        "lag_7": [5, 6, 10, 4, 7],
        "rolling_mean_7": [8.0, 3.0, 12.0, 1.0, 10.0],
        "rolling_std_7": [1.0, 1.0, 1.0, 1.0, 1.0],
        "rolling_sum_7": [56.0, 21.0, 84.0, 7.0, 70.0],
        "rolling_mean_30": [8.0, 3.0, 12.0, 1.0, 10.0],
        "rolling_std_30": [1.0, 1.0, 1.0, 1.0, 1.0],
        "rolling_sum_30": [56.0, 21.0, 84.0, 7.0, 70.0],
        "ema_7": [8.0, 3.0, 12.0, 1.0, 10.0],
        "ema_30": [8.0, 3.0, 12.0, 1.0, 10.0],
        "growth_rate": [0.1, -0.2, 0.1, 0.0, 0.2],
        "current_stock": [5, 45, 100, 1, 12],
        "unit_cost": [50.0, 50.0, 50.0, 50.0, 50.0],
        "reorder_point": [10, 10, 10, 10, 10],
        "lead_time": [7, 7, 7, 7, 7],
        "order_cost": [20.0, 20.0, 20.0, 20.0, 20.0],
        "holding_cost": [12.5, 12.5, 12.5, 12.5, 12.5],
        "average_usage": [8.0, 3.0, 12.0, 1.0, 10.0],
        "demand_trend": [0.5, -0.1, 0.4, 0.0, 0.3],
        "target": ["Low", "Healthy", "Overstock", "Critical", "Healthy"]
    }
    df = pd.DataFrame(data)
    feature_cols = [c for c in df.columns if c != "target"]
    
    clf, scaler, acc = train_classifier(df, feature_cols)
    assert clf is not None
    assert scaler is not None
    assert acc > 0.0
    
    # Predict single row
    row = df.iloc[0].to_dict()
    status, conf = predict_health(clf, scaler, row, feature_cols)
    assert status in ["Critical", "Low", "Healthy", "Overstock"]
    assert 0.0 <= conf <= 100.0

def test_eoq():
    """Validates economic order quantity calculation including edge cases."""
    # Standard values
    eoq = calculate_eoq(demand=10.0, ordering_cost=20.0, holding_cost_rate=0.25, unit_cost=50.0)
    assert eoq == 108
    
    # Edge Case: Division by zero / zero cost
    assert calculate_eoq(demand=10.0, ordering_cost=20.0, holding_cost_rate=0.0, unit_cost=50.0) == 0
    assert calculate_eoq(demand=10.0, ordering_cost=20.0, holding_cost_rate=0.25, unit_cost=-10.0) == 0

def test_priority_score():
    """Tests priority scoring weights and bounding constraints."""
    score_critical = calculate_priority_score(
        forecast_7d=50.0,
        current_stock=0,
        reorder_point=10,
        lead_time=7,
        trend=1.5,
        average_usage=7.0,
        stock_status="Critical"
    )
    score_safe = calculate_priority_score(
        forecast_7d=2.0,
        current_stock=150,
        reorder_point=10,
        lead_time=7,
        trend=-0.5,
        average_usage=1.0,
        stock_status="Overstock"
    )
    
    assert 0.0 <= score_critical <= 100.0
    assert 0.0 <= score_safe <= 100.0
    assert score_critical > score_safe

def test_explainability():
    """Checks generated recommendation strings."""
    explanation = generate_explanation(
        sku="TEST-SKU",
        name="Milk",
        current_stock=2,
        reorder_point=10,
        forecast_7d=42.5,
        lead_time=6,
        trend=0.1,
        eoq=50,
        priority="Critical",
        stock_status="Critical"
    )
    assert "Milk" in explanation
    assert "critically low" in explanation or "below the reorder point" in explanation
    assert "42.5" in explanation
    assert "EOQ" in explanation

def test_evaluation_metrics():
    """Verifies forecast and classification accuracy calculations."""
    y_true_fcst = [10.0, 20.0, 15.0]
    y_pred_fcst = [11.0, 19.0, 15.0]
    
    fcst_metrics = evaluate_forecast(y_true_fcst, y_pred_fcst)
    assert "mape" in fcst_metrics
    assert "rmse" in fcst_metrics
    assert fcst_metrics["rmse"] > 0
    
    # Zero items evaluation checks
    empty_fcst = evaluate_forecast([], [])
    assert empty_fcst["mape"] == 0.0
    
    y_true_clf = ["Healthy", "Low", "Critical"]
    y_pred_clf = ["Healthy", "Low", "Healthy"]
    clf_metrics = evaluate_classifier(y_true_clf, y_pred_clf)
    assert "accuracy" in clf_metrics
    assert clf_metrics["accuracy"] == pytest.approx(2.0 / 3.0)
    
    empty_clf = evaluate_classifier([], [])
    assert empty_clf["accuracy"] == 0.0

def test_model_manager(tmp_path):
    """Tests model manager saving, loading, and cache conditions."""
    from backend.config import settings
    original_model_dir = settings.MODEL_DIR
    
    # Setup temporary directory for test model saving
    settings.MODEL_DIR = str(tmp_path / "models")
    
    try:
        from sklearn.ensemble import RandomForestClassifier
        from sklearn.preprocessing import StandardScaler
        
        clf = RandomForestClassifier(n_estimators=5, random_state=42)
        clf.fit([[1, 2], [3, 4]], ["A", "B"])
        scaler = StandardScaler()
        scaler.fit([[1, 2], [3, 4]])
        
        # Test should_retrain on non-existent files
        assert should_retrain("hash1")
        
        # Save model
        save_model(clf, scaler, ["f1", "f2"], "hash1")
        
        # Check should_retrain with same hash vs different hash
        assert not should_retrain("hash1")
        assert should_retrain("hash2")
        
        # Load model
        loaded_clf, loaded_scaler, meta = load_model()
        assert loaded_clf is not None
        assert meta["data_hash"] == "hash1"
        
    finally:
        # Restore settings
        settings.MODEL_DIR = original_model_dir

def test_prediction_engine_cold_start():
    """Tests prediction engine with cold-start products."""
    products = [{"sku": "COLD", "name": "Cold Item", "current_stock": 5, "unit_cost": 2.0, "reorder_point": 10, "id": 1}]
    
    # Empty history
    empty_df = pd.DataFrame(columns=["sku", "name", "category", "sales_volume", "date", "product_id"])
    res = run_ml_pipeline(empty_df, products)
    assert len(res) == 0
    
    # 2 weeks history (<4 weeks)
    history = pd.DataFrame({
        "sku": ["COLD", "COLD"],
        "name": ["Cold Item", "Cold Item"],
        "category": ["A", "A"],
        "sales_volume": [2, 3],
        "date": ["2026-07-01", "2026-07-08"],
        "product_id": [1, 1]
    })
    
    res = run_ml_pipeline(history, products)
    assert len(res) == 1
    assert res[0].sku == "COLD"
    assert "cold-start" in res[0].explanations
    assert res[0].confidence == 50.0
