"""
inventra_bridge.py — Unified Python entry point for the C++ PythonBridge.

Called by PythonBridge::runSingleProduct() and PythonBridge::runAllProducts().
All functions return plain Python dicts so pybind11 can convert them without
requiring C++ to know about the MLResult dataclass internals.
"""

import json
import sys
import os
from datetime import datetime
from typing import List, Dict, Any, Optional

# ── Path setup (called from build/python/ where backend/ is also copied) ──────
_here = os.path.dirname(os.path.abspath(__file__))
_root = os.path.dirname(_here)   # build/ directory
for _p in (_here, _root):
    if _p not in sys.path:
        sys.path.insert(0, _p)

from backend.database.db_manager import DBManager
from backend.services.prediction_service import PredictionService
from backend.utils.logger import get_logger

logger = get_logger("inventra_bridge")


# ─────────────────────────────────────────────────────────────────────────────
# _mlresult_to_dict
# Convert a backend MLResult dataclass to a plain dict for pybind11 transfer.
# ─────────────────────────────────────────────────────────────────────────────

def _mlresult_to_dict(r) -> Dict[str, Any]:
    """Serialize an MLResult dataclass into a plain dict."""
    return {
        "product_id":   r.product_id,
        "sku":          r.sku,
        "demand_label": r.demand_label,
        "stock_status": r.stock_status,
        "confidence":   float(r.confidence),
        "forecast_7d":  float(r.forecast_7d),
        "trend":        float(r.trend),
        "eoq":          int(r.eoq),
        "priority":     r.priority,
        "explanation":  r.explanations if hasattr(r, "explanations") else "",
        "recommendation": _build_recommendation(r),
        "forecast_points": r.forecast_points if isinstance(r.forecast_points, list) else [],
    }


def _build_recommendation(r) -> str:
    """
    Generate a short, actionable recommendation string from the MLResult.
    This is displayed as a 1-2 sentence summary in the product detail panel.
    """
    status  = getattr(r, "stock_status", "")
    demand  = getattr(r, "demand_label", "")
    eoq     = int(getattr(r, "eoq", 0))
    forecast= float(getattr(r, "forecast_7d", 0.0))
    priority= getattr(r, "priority", "Safe")

    if status in ("Critical", "Reorder") or priority == "Critical":
        return (
            f"⚠️ Immediate reorder required. Place an order of approximately "
            f"{eoq} units to meet forecast demand of {forecast:.0f} units over the next 7 days."
        )
    elif priority == "Reorder Soon" or status in ("Low", "Reorder"):
        return (
            f"Stock levels are approaching the reorder point. "
            f"Consider placing an order of {eoq} units soon. "
            f"Forecasted demand for the next 7 days: {forecast:.0f} units."
        )
    elif status == "Overstock":
        return (
            f"Stock is above optimal levels. Hold off on new orders. "
            f"Forecasted consumption over the next 7 days: {forecast:.0f} units."
        )
    else:
        return (
            f"Inventory is at a healthy level. {demand} demand detected. "
            f"Next reorder of {eoq} units recommended when stock nears the reorder point."
        )


# ─────────────────────────────────────────────────────────────────────────────
# predict_all_products
# Called by PythonBridge::runPipeline() when csvPath is empty (DB mode).
# ─────────────────────────────────────────────────────────────────────────────

def predict_all_products(
    db_path:           str,
    lead_time_days:    int   = 7,
    ordering_cost:     float = 20.0,
    holding_cost_rate: float = 0.25,
    forecast_horizon:  int   = 7
) -> List[Dict[str, Any]]:
    """
    Load all products from SQLite, run the full ML pipeline, save results,
    and return a list of dicts with prediction data.

    Called from PythonBridge::runPipeline() with an empty csvPath.
    """
    logger.info(f"predict_all_products: db={db_path}")

    db = DBManager(db_path)
    service = PredictionService(db)

    results = service.run_predictions(
        lead_time_days=lead_time_days,
        ordering_cost=ordering_cost,
        holding_cost_rate=holding_cost_rate,
        forecast_horizon=forecast_horizon,
    )

    return [_mlresult_to_dict(r) for r in results]


# ─────────────────────────────────────────────────────────────────────────────
# predict_single_product
# Called by PythonBridge::runSingleProduct() for on-demand per-product prediction.
# ─────────────────────────────────────────────────────────────────────────────

def predict_single_product(
    product_id:        int,
    db_path:           str,
    lead_time_days:    int   = 7,
    ordering_cost:     float = 20.0,
    holding_cost_rate: float = 0.25,
    forecast_horizon:  int   = 7
) -> Optional[Dict[str, Any]]:
    """
    Run ML prediction for a single product identified by product_id.
    Loads that product's sales history from the DB and returns one result dict.

    Returns None if the product is not found.
    """
    import pandas as pd
    from backend.ml.prediction_engine import run_ml_pipeline

    logger.info(f"predict_single_product: product_id={product_id}, db={db_path}")

    db = DBManager(db_path)

    # 1. Load the specific product
    all_products = db.list_products()
    target = next((p for p in all_products if p.id == product_id), None)
    if target is None:
        logger.warning(f"Product id={product_id} not found in DB.")
        return None

    products_list = [{
        "id":            target.id,
        "sku":           target.sku,
        "name":          target.name,
        "category":      target.category,
        "current_stock": target.current_stock,
        "unit_cost":     target.unit_cost,
        "reorder_point": target.reorder_point,
    }]

    # 2. Load historical sales for this product only
    import sqlite3
    conn = sqlite3.connect(db_path)
    conn.row_factory = sqlite3.Row
    cur = conn.cursor()
    cur.execute("""
        SELECT p.sku, p.name, p.category,
               de.units_sold AS sales_volume,
               de.entry_date AS date,
               p.id          AS product_id
        FROM daily_entries de
        JOIN products p ON de.product_id = p.id
        WHERE p.id = ?
        ORDER BY de.entry_date ASC;
    """, (product_id,))
    rows = [dict(r) for r in cur.fetchall()]
    conn.close()

    if not rows:
        logger.warning(f"No daily entries for product_id={product_id}. Using cold-start.")
        # Build a minimal one-row dataframe so cold-start path kicks in
        rows = [{
            "sku": target.sku, "name": target.name, "category": target.category,
            "sales_volume": 0, "date": datetime.now().strftime("%Y-%m-%d"),
            "product_id": product_id
        }]

    sales_df = pd.DataFrame(rows)

    # 3. Run pipeline for just this product
    results = run_ml_pipeline(
        sales_history_df=sales_df,
        products_list=products_list,
        lead_time_days=lead_time_days,
        ordering_cost=ordering_cost,
        holding_cost_rate=holding_cost_rate,
        forecast_horizon=forecast_horizon,
    )

    if not results:
        return None

    result_dict = _mlresult_to_dict(results[0])

    # 4. Save single-product result to pipeline_results for persistence
    try:
        run_at = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

        # Read most recent pipeline run and update/append this product
        existing_rows = db.get_latest_pipeline_results(limit=1)
        existing = []
        if existing_rows and existing_rows[0].get("success"):
            try:
                existing = json.loads(existing_rows[0].get("results_json", "[]"))
            except Exception:
                existing = []

        # Replace entry for this product if already present
        existing = [e for e in existing if e.get("product_id") != product_id]
        existing.append({
            "product_id":      result_dict["product_id"],
            "sku":             result_dict["sku"],
            "demand_label":    result_dict["demand_label"],
            "stock_status":    result_dict["stock_status"],
            "confidence":      result_dict["confidence"],
            "forecast_7d":     result_dict["forecast_7d"],
            "trend":           result_dict["trend"],
            "eoq":             result_dict["eoq"],
            "priority":        result_dict["priority"],
            "explanations":    result_dict["explanation"],
            "forecast_points": result_dict["forecast_points"],
        })
        db.save_pipeline_result(run_at, True, json.dumps(existing))
    except Exception as e:
        logger.warning(f"Could not persist single-product result: {e}")

    return result_dict

# ─────────────────────────────────────────────────────────────────────────────
# import_csv_and_predict
# Called by PythonBridge::runPipeline() when csvPath is provided.
# Emulates the CLI import_dataset.py logic to actually inject data into SQLite.
# ─────────────────────────────────────────────────────────────────────────────
import pandas as pd
import sqlite3

def import_csv_and_predict(csv_path: str, db_path: str) -> List[Dict[str, Any]]:
    logger.info(f"Importing dataset from {csv_path} to DB: {db_path}")
    df = pd.read_csv(csv_path)

    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    # Clear old data
    cursor.execute("DELETE FROM pipeline_results")
    cursor.execute("DELETE FROM daily_entries")
    cursor.execute("DELETE FROM stock_movements")
    cursor.execute("DELETE FROM products")
    conn.commit()

    # Insert Products
    latest_df = df.sort_values('week_start_date').groupby('product_id').last().reset_index()
    product_id_map = {}
    for _, row in latest_df.iterrows():
        cursor.execute(
            "INSERT INTO products (sku, name, category, current_stock, unit_cost, reorder_point, "
            "created_at, updated_at) VALUES (?, ?, ?, ?, ?, ?, datetime('now'), datetime('now'))",
            (str(row['product_id']), str(row['product_name']), str(row['category']),
             int(row['closing_stock']), float(row['unit_price_inr']), int(row['reorder_point']))
        )
        product_id_map[row['product_id']] = cursor.lastrowid

    conn.commit()
    
    # Insert Daily Entries
    for _, row in df.iterrows():
        db_pid = product_id_map.get(row['product_id'])
        if db_pid:
            cursor.execute(
                "INSERT INTO daily_entries (product_id, entry_date, units_sold, units_wasted, "
                "entered_by, created_at) VALUES (?, ?, ?, 0, 1, datetime('now'))",
                (db_pid, str(row['week_start_date']), int(row['weekly_sales']))
            )
    conn.commit()
    conn.close()

    logger.info("Dataset loaded successfully. Triggering predict_all_products.")
    
    # Now run prediction for all
    raw_results = predict_all_products(db_path)
    
    # Ensure they have recommendations and save them properly
    conn = sqlite3.connect(db_path)
    conn.execute("DELETE FROM pipeline_results") # remove partial from PredictionService
    
    enriched = []
    for r in raw_results:
        if not r.get("recommendation"):
            r["recommendation"] = _build_recommendation(r)
        if "explanation" not in r and "explanations" in r:
            r["explanation"] = r["explanations"]
        enriched.append(r)
        
    run_at = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    conn.execute(
        "INSERT INTO pipeline_results (run_at, success, results_json) VALUES (?, 1, ?)",
        (run_at, json.dumps(enriched))
    )
    conn.commit()
    conn.close()
    
    return enriched

