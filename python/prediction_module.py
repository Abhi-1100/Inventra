"""
C++ Python Bridge Wrapper for demand forecasting and EOQ optimization.
Delegates to backend ML forecasting, EOQ, and priority ranking engines.
"""

import pandas as pd
import numpy as np
from datetime import datetime
from typing import List, Dict, Any

from backend.ml.forecasting import forecast_demand
from backend.ml.eoq import calculate_eoq
from backend.ml.priority import calculate_priority_score

def calculate_eoq_val(demand: float, ordering_cost: float, holding_cost_rate: float, unit_cost: float) -> int:
    """
    Standard Wilson EOQ Formula.
    """
    return calculate_eoq(demand, ordering_cost, holding_cost_rate, unit_cost)

def forecast_all(
    df: pd.DataFrame,
    ml_results: dict,
    horizon: int,
    ordering_cost: float,
    holding_cost_rate: float
) -> list:
    """
    Performs demand forecasting and EOQ calculations.
    Returns a list of prediction records matching C++ PythonBridge structs.
    """
    forecast_results = []
    unique_skus = df['sku'].unique()
    today = datetime.now()
    
    for sku in unique_skus:
        sku_df = df[df['sku'] == sku]
        
        # Retrieve ML classification info from step 2
        ml_data = ml_results.get(sku, {
            "demand_label": "Medium",
            "stock_status": "NoAction",
            "confidence": 50.0,
            "avg_sales": 5.0,
            "current_stock": 0,
            "unit_cost": 0.0
        })
        
        avg_sales = float(ml_data.get("avg_sales", 5.0))
        current_stock = int(ml_data.get("current_stock", 0))
        unit_cost = float(ml_data.get("unit_cost", 0.0))
        
        # 1. Run Forecasting
        forecast_7d, trend, forecast_pts, _ = forecast_demand(sku_df, horizon, today)
        
        # 2. Run EOQ
        eoq = calculate_eoq(avg_sales, ordering_cost, holding_cost_rate, unit_cost)
        
        # 3. Calculate Priority score
        # Reorder point is taken from the last row of dataframe or defaulted to 10
        reorder_point = int(sku_df.iloc[-1].get("reorder_point", 10)) if not sku_df.empty else 10
        lead_time = int(sku_df.iloc[-1].get("lead_time", 7)) if not sku_df.empty else 7
        
        priority_score = calculate_priority_score(
            forecast_7d=forecast_7d,
            current_stock=current_stock,
            reorder_point=reorder_point,
            lead_time=lead_time,
            trend=trend,
            average_usage=avg_sales,
            stock_status=ml_data["stock_status"]
        )
        
        priority_label = "Safe"
        if priority_score >= 75.0:
            priority_label = "Critical"
        elif priority_score >= 50.0:
            priority_label = "ReorderSoon"
            
        # Get product_id if exists
        product_id = 0
        if "product_id" in sku_df.columns and not sku_df.empty:
            product_id = int(sku_df["product_id"].iloc[0])
            
        forecast_results.append({
            "product_id": product_id,
            "sku": sku,
            "demand_label": ml_data["demand_label"],
            "stock_status": ml_data["stock_status"],
            "confidence": ml_data["confidence"],
            "forecast_7d": forecast_7d,
            "trend": trend,
            "eoq": eoq,
            "priority": priority_label,
            "forecast_points": forecast_pts
        })
        
    return forecast_results
