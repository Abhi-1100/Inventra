"""
C++ Python Bridge Wrapper for inventory classification.
Delegates to backend ML classifier and feature engineering engines.
"""

import pandas as pd
import numpy as np
from typing import Dict, Any

from backend.ml.prediction_engine import run_ml_pipeline
from backend.models.entities import MLResult

def run_pipeline(df: pd.DataFrame) -> dict:
    """
    Executes demand status classification pipeline.
    Returns a dict mapping SKU to predictions expected by C++ PythonBridge.
    """
    results = {}
    
    # Extract unique products from the DataFrame
    products_list = []
    for sku, sku_df in df.groupby("sku"):
        last_row = sku_df.iloc[-1]
        
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
        
    # Execute ML pipeline using default settings
    pipeline_results = run_ml_pipeline(
        sales_history_df=df,
        products_list=products_list
    )
    
    # Format outputs as expected by C++ (sku -> dict)
    for r in pipeline_results:
        # Find the product details
        sku_df = df[df["sku"] == r.sku]
        avg_sales = float(sku_df["sales_volume"].mean()) if not sku_df.empty else 5.0
        last_row = sku_df.iloc[-1] if not sku_df.empty else {}
        
        current_stock = int(last_row.get("current_stock", 0)) if not sku_df.empty else 0
        unit_cost = float(last_row.get("unit_cost", 0.0)) if not sku_df.empty else 0.0
        
        # In the C++ side, StockStatus:Reorder = "Reorder", Overstock = "Overstock", NoAction = "NoAction" (or falls back to NoAction)
        # Note: in PythonBridge.cpp, ss == "Reorder" -> Reorder, ss == "Overstock" -> Overstock, else NoAction.
        # So we can output "Reorder", "Overstock", or "NoAction".
        results[r.sku] = {
            "demand_label": r.demand_label,
            "stock_status": r.stock_status,
            "confidence": r.confidence,
            "avg_sales": avg_sales,
            "current_stock": current_stock,
            "unit_cost": unit_cost
        }
        
    return results
