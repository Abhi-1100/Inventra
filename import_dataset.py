"""
import_dataset.py — Import kirana_inventory_dataset_v2_polished.csv into SQLite
and run ML predictions for all 100 products.

Usage: python3 import_dataset.py
"""

import pandas as pd
import sqlite3
import json
import sys
import os
from datetime import datetime

# ── Path setup ─────────────────────────────────────────────────────────────
_dir = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(_dir, 'build', 'python'))
sys.path.insert(0, os.path.join(_dir, 'build'))
sys.path.insert(0, _dir)  # so 'backend' package is found

CSV_PATH = "/Users/ghoriommukeshbai/Desktop/research paper/model/kirana_inventory_dataset_v2_polished.csv"
DB_PATH  = os.path.join(_dir, "build", "inventra.db")


def _build_recommendation(stock_status: str, priority: str, eoq: int, forecast: float) -> str:
    """Short actionable recommendation sentence for the detail panel."""
    if priority == "Critical" or stock_status in ("Critical", "Low"):
        return (
            f"\u26a0\ufe0f Immediate reorder required. Place an order of approximately "
            f"{eoq} units to meet the forecast demand of {forecast:.0f} units over the next 7 days."
        )
    elif priority == "Reorder Soon" or stock_status == "Reorder":
        return (
            f"Stock approaching reorder point. Consider ordering {eoq} units soon. "
            f"7-day forecast: {forecast:.0f} units."
        )
    elif stock_status == "Overstock":
        return (
            f"Stock above optimal levels. Hold off on new orders. "
            f"Expected consumption next 7 days: {forecast:.0f} units."
        )
    else:
        return (
            f"Inventory at a healthy level. Next reorder of {eoq} units recommended "
            f"when stock nears the reorder point. 7-day forecast: {forecast:.0f} units."
        )


def import_data():
    print(f"[1/4] Loading CSV from {CSV_PATH} ...")
    df = pd.read_csv(CSV_PATH)
    print(f"      Shape: {df.shape} — {df['product_id'].nunique()} unique products")

    # ── Step 1: Clear and re-seed database ───────────────────────────────────
    print("[2/4] Importing products and sales history into SQLite ...")
    conn = sqlite3.connect(DB_PATH)
    cursor = conn.cursor()

    cursor.execute("DELETE FROM pipeline_results")
    cursor.execute("DELETE FROM daily_entries")
    cursor.execute("DELETE FROM stock_movements")
    cursor.execute("DELETE FROM products")
    conn.commit()

    # Insert Products (use last week's closing_stock as current stock)
    latest_df = df.sort_values('week_start_date').groupby('product_id').last().reset_index()
    product_id_map = {}

    for _, row in latest_df.iterrows():
        cursor.execute(
            "INSERT INTO products (sku, name, category, current_stock, unit_cost, reorder_point, "
            "created_at, updated_at) VALUES (?, ?, ?, ?, ?, ?, datetime('now'), datetime('now'))",
            (
                str(row['product_id']),
                str(row['product_name']),
                str(row['category']),
                int(row['closing_stock']),
                float(row['unit_price_inr']),
                int(row['reorder_point']),
            )
        )
        product_id_map[row['product_id']] = cursor.lastrowid

    conn.commit()
    print(f"      Inserted {len(product_id_map)} products.")

    # Insert Daily Entries (one entry per week per product = 50 weeks each)
    entries = 0
    for _, row in df.iterrows():
        db_pid = product_id_map.get(row['product_id'])
        if db_pid is None:
            continue
        cursor.execute(
            "INSERT INTO daily_entries (product_id, entry_date, units_sold, units_wasted, "
            "entered_by, created_at) VALUES (?, ?, ?, 0, 1, datetime('now'))",
            (db_pid, str(row['week_start_date']), int(row['weekly_sales']))
        )
        entries += 1

    conn.commit()
    conn.close()
    print(f"      Inserted {entries} sales entries.")

    # ── Step 2: Run ML pipeline ───────────────────────────────────────────────
    print("[3/4] Running ML pipeline (Prophet + SVM + EOQ) ...")
    from inventra_bridge import predict_all_products
    
    raw_results = predict_all_products(DB_PATH)
    print(f"      ML pipeline generated {len(raw_results)} predictions.")

    # ── Step 3: Enrich with recommendation and save with complete schema ──────
    print("[4/4] Saving enriched predictions to SQLite ...")

    # Delete any partial results saved by PredictionService during the run
    conn = sqlite3.connect(DB_PATH)
    conn.execute("DELETE FROM pipeline_results")
    conn.commit()

    enriched = []
    for r in raw_results:
        # r already has 'recommendation' from inventra_bridge._mlresult_to_dict
        # but let's make sure it's non-empty
        if not r.get("recommendation"):
            r["recommendation"] = _build_recommendation(
                r.get("stock_status", "Healthy"),
                r.get("priority", "Safe"),
                int(r.get("eoq", 0)),
                float(r.get("forecast_7d", 0.0)),
            )
        # Also ensure 'explanation' key exists (alias of 'explanations')
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

    # ── Verify ────────────────────────────────────────────────────────────────
    conn = sqlite3.connect(DB_PATH)
    c    = conn.cursor()
    c.execute("SELECT COUNT(*) FROM products");      print(f"\n✓ Products in DB:         {c.fetchone()[0]}")
    c.execute("SELECT COUNT(*) FROM daily_entries"); print(f"✓ Daily entries in DB:    {c.fetchone()[0]}")
    c.execute("SELECT COUNT(*) FROM pipeline_results WHERE success=1")
    print(f"✓ Pipeline result rows:   {c.fetchone()[0]}")
    c.execute("SELECT results_json FROM pipeline_results WHERE success=1 ORDER BY run_at DESC LIMIT 1")
    row = c.fetchone()
    results = json.loads(row[0])
    print(f"✓ Predictions stored:     {len(results)}")
    sample = results[0]
    has_rec  = bool(sample.get("recommendation"))
    has_exp  = bool(sample.get("explanations") or sample.get("explanation"))
    has_fcst = len(sample.get("forecast_points", [])) > 0
    print(f"✓ Has recommendation:     {has_rec}")
    print(f"✓ Has explanation:        {has_exp}")
    print(f"✓ Has forecast_points:    {has_fcst} ({len(sample.get('forecast_points',[]))} pts)")
    from collections import Counter
    print(f"✓ Stock statuses:         {dict(Counter(r['stock_status'] for r in results))}")
    print(f"✓ Priorities:             {dict(Counter(r['priority'] for r in results))}")
    print(f"✓ Demand labels:          {dict(Counter(r['demand_label'] for r in results))}")
    print(f"\n✅ Import and prediction complete. Launch the app to see live data.")
    conn.close()


if __name__ == "__main__":
    import_data()
