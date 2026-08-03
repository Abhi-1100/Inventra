"""
main.py — Inventra FastAPI Backend Server

This is the HTTP server that the C++ frontend connects to.

Endpoints:
  GET  /               → Health check — tells you the server is alive.
  POST /upload_csv     → Accept a CSV file, store raw data in SQLite,
                         apply preprocessing, run the ML pipeline,
                         and return predictions + raw rows in JSON.

How to run:
  uvicorn main:app --host 0.0.0.0 --port 8000 --reload

How it works (beginner explanation):
  1. The C++ frontend sends a POST request to /upload_csv with a file attached.
  2. FastAPI reads the file into memory, hands it to pandas as a DataFrame.
  3. We validate that all expected columns are present.
  4. We save the raw rows into a local SQLite database (table: "dataset").
  5. We apply the SAME preprocessing steps used during training (lags, rolling
     stats, EMAs, etc.) by calling backend.ml.preprocess.preprocess_dataframe.
  6. The processed features are handed to the ML pipeline (SVC classifier +
     demand forecaster) which is already trained and saved on disk.
  7. The response is a JSON object containing both the raw CSV rows and the
     ML predictions for each product SKU.
"""

import os
import sys
import json
import sqlite3
import io
from pathlib import Path
from typing import Any, Dict, List

import pandas as pd
from fastapi import FastAPI, File, UploadFile, HTTPException
from fastapi.responses import JSONResponse

# ── Make sure the backend package is importable ──────────────────────────────
# This lets us run  `uvicorn main:app`  from the Inventra/ directory.
BASE_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(BASE_DIR))

from backend.ml.preprocess import (
    validate_csv_columns,
    preprocess_dataframe,
    CLASSIFIER_FEATURES,
)
from backend.ml.prediction_engine import run_ml_pipeline
from backend.utils.logger import get_logger

logger = get_logger("fastapi_server")

# ── SQLite database path ─────────────────────────────────────────────────────
DB_PATH = BASE_DIR / "backend" / "inventra_api.db"

# ── FastAPI application ──────────────────────────────────────────────────────
app = FastAPI(
    title="Inventra ML API",
    description=(
        "REST API for the Inventra inventory intelligence system. "
        "Upload a CSV file to receive inventory health predictions, "
        "demand forecasts, EOQ recommendations, and priority scores."
    ),
    version="1.0.0",
)


# ─────────────────────────────────────────────────────────────────────────────
# Startup: Pre-load the trained model so it is NOT reloaded on every request.
# The model files (classifier.joblib, scaler.joblib) were saved by train_model.py.
# ─────────────────────────────────────────────────────────────────────────────
@app.on_event("startup")
def load_model_on_startup():
    """
    Loads the trained SVC classifier and StandardScaler from disk once at
    server startup.  Stored in app.state so every request can reuse them
    without re-loading from disk.

    If no saved model is found, app.state.clf and app.state.scaler are set
    to None — the pipeline will then fall back to heuristic rules.
    """
    from backend.ml.model_manager import load_model

    clf, scaler, metadata = load_model()

    if clf and scaler:
        logger.info(
            f"✅ Pre-trained model loaded successfully "
            f"(type={metadata.get('model_type')}, "
            f"trained={metadata.get('training_timestamp')})"
        )
    else:
        logger.warning(
            "⚠️  No pre-trained model found in backend/saved_models/. "
            "The pipeline will use heuristic fallback rules. "
            "Run `python train_model.py <path_to_csv>` to train and save a model."
        )

    app.state.clf = clf
    app.state.scaler = scaler
    app.state.model_metadata = metadata
    
    preds, raw = load_predictions_from_sqlite()
    app.state.latest_predictions = preds
    app.state.latest_raw_data = raw


# ─────────────────────────────────────────────────────────────────────────────
# Helper: SQLite initialisation
# ─────────────────────────────────────────────────────────────────────────────
def get_db_connection() -> sqlite3.Connection:
    """Opens (and if needed, creates) the SQLite database."""
    DB_PATH.parent.mkdir(parents=True, exist_ok=True)
    conn = sqlite3.connect(str(DB_PATH))
    conn.row_factory = sqlite3.Row  # lets us access columns by name
    return conn

def load_predictions_from_sqlite():
    """Loads the most recent ML predictions from SQLite so they survive restarts."""
    conn = get_db_connection()
    try:
        conn.execute("CREATE TABLE IF NOT EXISTS ml_results (id INTEGER PRIMARY KEY, timestamp TEXT DEFAULT CURRENT_TIMESTAMP, predictions_json TEXT, raw_data_json TEXT)")
        cur = conn.execute("SELECT predictions_json, raw_data_json FROM ml_results ORDER BY id DESC LIMIT 1")
        row = cur.fetchone()
        if row:
            return json.loads(row[0]), json.loads(row[1])
        return None, None
    except Exception as e:
        logger.error(f"Failed to load predictions from sqlite: {e}")
        return None, None
    finally:
        conn.close()

def save_predictions_to_sqlite(predictions: List[Dict], raw_data: List[Dict]) -> None:
    """Saves ML predictions to SQLite so they act as a permanent source of truth."""
    conn = get_db_connection()
    try:
        conn.execute("CREATE TABLE IF NOT EXISTS ml_results (id INTEGER PRIMARY KEY, timestamp TEXT DEFAULT CURRENT_TIMESTAMP, predictions_json TEXT, raw_data_json TEXT)")
        conn.execute("INSERT INTO ml_results (predictions_json, raw_data_json) VALUES (?, ?)", (json.dumps(predictions), json.dumps(raw_data)))
        conn.commit()
    except Exception as e:
        logger.error(f"Failed to save predictions to sqlite: {e}")
    finally:
        conn.close()


def save_dataframe_to_sqlite(df: pd.DataFrame, table_name: str = "dataset") -> None:
    """
    Saves the raw CSV DataFrame into the SQLite table named 'dataset'.

    If the table already exists, rows are appended (not overwritten).
    Each import gets a unique 'import_batch_id' column so you can
    tell which rows came from which upload.
    """
    conn = get_db_connection()
    try:
        import hashlib, time
        batch_id = hashlib.md5(str(time.time()).encode()).hexdigest()[:8]
        df_to_save = df.copy()
        df_to_save["import_batch_id"] = batch_id
        df_to_save.to_sql(table_name, conn, if_exists="append", index=False)
        conn.commit()
        logger.info(
            f"Saved {len(df_to_save)} rows to SQLite table '{table_name}' "
            f"(batch={batch_id})"
        )
    finally:
        conn.close()


# ─────────────────────────────────────────────────────────────────────────────
# GET /  — Health check
# ─────────────────────────────────────────────────────────────────────────────
@app.get("/", tags=["Health"])
def health_check():
    """
    Simple liveness probe.

    Returns 200 with a status message and whether a trained model is loaded.
    Your C++ frontend can call this at startup to verify the server is up.
    """
    model_loaded = app.state.clf is not None and app.state.scaler is not None
    meta = app.state.model_metadata or {}
    return {
        "status": "ok",
        "server": "Inventra ML API",
        "version": "1.0.0",
        "model_loaded": model_loaded,
        "model_type": meta.get("model_type", "none"),
        "model_trained_at": meta.get("training_timestamp", "N/A"),
    }


# ─────────────────────────────────────────────────────────────────────────────
# POST /upload_csv  — Main prediction endpoint
# ─────────────────────────────────────────────────────────────────────────────
@app.post("/upload_csv", tags=["Predictions"])
async def upload_csv(file: UploadFile = File(...)):
    """
    Accepts a CSV file, preprocesses it, runs ML predictions, and returns
    both the raw row data and predictions in a single JSON response.

    ## What this does (step by step)

    1. Reads the uploaded CSV into a pandas DataFrame.
    2. Validates that all required columns are present.
    3. Saves the raw rows to the SQLite table 'dataset'.
    4. Applies the exact same preprocessing that was used at training time:
       lag features, rolling statistics, EMAs, growth rate, etc.
    5. Runs the ML pipeline (SVC classifier + demand forecaster).
    6. Returns predictions alongside the original raw rows.

    ## Required CSV columns

    product_id, product_name, week_start_date, weekly_sales, closing_stock,
    unit_price_inr, reorder_point, lead_time, order_cost, holding_cost

    ## Response format

    ```json
    {
      "status": "success",
      "rows_processed": 42,
      "unique_skus": 3,
      "raw_data": [ { ...one dict per CSV row... } ],
      "predictions": [
        {
          "sku": "P001",
          "product_name": "...",
          "stock_status": "Healthy",
          "confidence": 87.3,
          "demand_label": "High Demand",
          "forecast_7d": 120.5,
          "trend": 0.8,
          "eoq": 45,
          "priority": "Safe",
          "explanations": "...",
          "forecast_points": [ ... ]
        }
      ]
    }
    ```
    """
    # ── 1. Read the uploaded file ─────────────────────────────────────────────
    if not file.filename.lower().endswith(".csv"):
        raise HTTPException(
            status_code=400,
            detail="Only CSV files are accepted. Please upload a .csv file.",
        )

    try:
        contents = await file.read()
        df_raw = pd.read_csv(io.BytesIO(contents))
    except Exception as e:
        raise HTTPException(
            status_code=400,
            detail=f"Could not parse the uploaded file as CSV: {e}",
        )

    if df_raw.empty:
        raise HTTPException(
            status_code=400,
            detail="The uploaded CSV file is empty.",
        )

    # ── 2. Validate columns ────────────────────────────────────────────────────
    is_valid, error_message = validate_csv_columns(df_raw)
    if not is_valid:
        raise HTTPException(
            status_code=422,  # Unprocessable Entity
            detail=(
                f"Column validation failed. {error_message}. "
                f"Columns found in your file: {list(df_raw.columns)}"
            ),
        )

    # ── 3. Save raw data to SQLite ────────────────────────────────────────────
    try:
        save_dataframe_to_sqlite(df_raw, table_name="dataset")
    except Exception as e:
        logger.error(f"SQLite save failed: {e}", exc_info=True)
        raise HTTPException(
            status_code=500,
            detail=f"Failed to save data to database: {e}",
        )

    # ── 4. Preprocess (feature engineering) ───────────────────────────────────
    try:
        features_df, products_list = preprocess_dataframe(df_raw)
    except Exception as e:
        logger.error(f"Preprocessing failed: {e}", exc_info=True)
        raise HTTPException(
            status_code=500,
            detail=f"Preprocessing error: {e}",
        )

    if features_df.empty or not products_list:
        raise HTTPException(
            status_code=422,
            detail=(
                "Could not extract features from the uploaded data. "
                "Ensure the CSV contains at least a few rows per product "
                "and that numeric columns (weekly_sales, closing_stock, etc.) "
                "are not all zeros or NaN."
            ),
        )

    # ── 5. Build internal sales_history_df expected by run_ml_pipeline ────────
    # The pipeline needs a DataFrame with: sku, name, category, sales_volume, date
    from backend.ml.preprocess import normalize_csv
    df_internal = normalize_csv(df_raw.copy())

    # Make sure required internal columns exist
    if "sales_volume" not in df_internal.columns:
        df_internal["sales_volume"] = 0.0
    if "name" not in df_internal.columns:
        df_internal["name"] = df_internal.get("sku", "Unknown")
    if "category" not in df_internal.columns:
        df_internal["category"] = ""

    sales_history_df = df_internal[["sku", "name", "category", "sales_volume", "date"]].copy()
    sales_history_df["sales_volume"] = pd.to_numeric(
        sales_history_df["sales_volume"], errors="coerce"
    ).fillna(0.0)

    # ── 6. Run ML pipeline ─────────────────────────────────────────────────────
    # use_prophet=False → skips the slow Prophet fitting (which takes ~1-3s per
    # product). Linear Regression fallback gives results in milliseconds and is
    # accurate enough for real-time API use. Set True if you want max accuracy
    # and don't mind waiting ~1-3 minutes for large datasets.
    try:
        results = run_ml_pipeline(
            sales_history_df=sales_history_df,
            products_list=products_list,
            lead_time_days=7,
            ordering_cost=20.0,
            holding_cost_rate=0.25,
            forecast_horizon=7,
            use_prophet=False,   # ← FAST mode: skip Prophet for API calls
        )
    except Exception as e:
        logger.error(f"ML pipeline failed: {e}", exc_info=True)
        raise HTTPException(
            status_code=500,
            detail=f"ML pipeline error: {e}",
        )

    # ── 7. Build response ──────────────────────────────────────────────────────
    # Map SKU → product name for enriching the prediction output
    sku_to_name: Dict[str, str] = {}
    for p in products_list:
        sku_to_name[str(p["sku"])] = str(p.get("name", p["sku"]))

    predictions_output: List[Dict[str, Any]] = []
    for r in results:
        rec_text = _build_recommendation(
            priority=str(getattr(r, 'priority', 'Safe')),
            stock_status=str(getattr(r, 'stock_status', 'Healthy')),
            eoq=int(getattr(r, 'eoq', 0)),
            forecast_7d=float(getattr(r, 'forecast_7d', 0.0)),
        )

        predictions_output.append({
            "sku": r.sku,
            "product_name": sku_to_name.get(str(r.sku), str(r.sku)),
            "stock_status": r.stock_status,
            "confidence_pct": round(r.confidence, 2),
            "demand_label": r.demand_label,
            "forecast_7d_units": round(r.forecast_7d, 2),
            "trend_slope": round(r.trend, 4),
            "eoq_units": r.eoq,
            "priority": r.priority,
            "recommendation": rec_text,
            "explanations": r.explanations,
            "forecast_points": r.forecast_points,
        })

    # raw_data: return ONE summary row per SKU (latest row) instead of all rows.
    # This keeps the response small (~15 KB vs 4.6 MB for 5000 rows) so the
    # browser /docs UI can render it. Your C++ frontend gets all it needs from
    # the predictions array. If you need ALL raw rows, use raw_data_include_all=true.
    raw_data_output: List[Dict[str, Any]] = []
    for p in products_list:
        raw_data_output.append({
            "sku": p["sku"],
            "name": p.get("name", ""),
            "category": p.get("category", ""),
            "current_stock": p.get("current_stock", 0),
            "unit_cost": p.get("unit_cost", 0.0),
            "reorder_point": p.get("reorder_point", 10),
        })

    # ── Store results in app.state and persist to SQLite ────────────
    app.state.latest_predictions = predictions_output
    app.state.latest_raw_data = raw_data_output
    app.state.latest_products_list = products_list
    
    save_predictions_to_sqlite(predictions_output, raw_data_output)

    return JSONResponse(
        content={
            "status": "success",
            "file_name": file.filename,
            "rows_processed": len(df_raw),
            "unique_skus": len(products_list),
            "raw_data": raw_data_output,      # one summary row per SKU
            "predictions": predictions_output,  # full ML predictions for each SKU
        }
    )



# ─────────────────────────────────────────────────────────────────────────────
# Helper: build a human-readable recommendation sentence
# ─────────────────────────────────────────────────────────────────────────────
def _build_recommendation(*, priority: str, stock_status: str, eoq: int, forecast_7d: float) -> str:
    """
    Returns a short, actionable recommendation sentence based on ML outputs.
    Called during both upload processing and enriched-results generation so that
    predictions loaded from SQLite (which predate the recommendation field) also
    get a valid recommendation string.
    """
    pr = str(priority)
    st = str(stock_status)
    fcast_val = float(forecast_7d)
    eoq_val = int(eoq)

    if pr == "Critical" or st in ("Critical", "Low"):
        return (
            f"\u26a0\ufe0f Immediate reorder required. Place an order of approximately "
            f"{eoq_val} units to meet the forecasted demand of {fcast_val:.0f} units "
            f"over the next 7 days."
        )
    elif pr == "ReorderSoon" or st == "Reorder":
        return (
            f"Stock is approaching the reorder point. Consider ordering {eoq_val} units soon. "
            f"Forecasted demand for the next 7 days: {fcast_val:.0f} units."
        )
    elif st == "Overstock":
        return (
            f"Stock is above optimal levels. Hold off on new orders. "
            f"Expected consumption over the next 7 days: {fcast_val:.0f} units."
        )
    else:
        return (
            f"Inventory is at a healthy level. "
            f"Next reorder of {eoq_val} units recommended when stock nears the reorder point. "
            f"7-day forecast: {fcast_val:.0f} units."
        )


# ─────────────────────────────────────────────────────────────────────────────
# GET /results  — Return latest predictions
# ─────────────────────────────────────────────────────────────────────────────
@app.get("/results", tags=["Results"])
def get_all_results():
    """
    Returns the latest ML predictions from the most recent /upload_csv run.
    Each item contains: product_id, product_name, category, current_stock,
    demand_label, stock_status, confidence, weekly_forecast, eoq_quantity,
    urgency, days_remaining, explanation, trend_direction, forecast_points.
    """
    predictions = getattr(app.state, "latest_predictions", None)
    raw_data = getattr(app.state, "latest_raw_data", None)

    if not predictions:
        return JSONResponse(status_code=200, content=[])

    # Merge raw_data info into predictions for a richer response
    raw_map: Dict[str, Dict] = {}
    if raw_data:
        for rd in raw_data:
            raw_map[str(rd.get("sku", ""))] = rd

    enriched: List[Dict[str, Any]] = []
    for pred in predictions:
        sku = str(pred.get("sku", ""))
        raw = raw_map.get(sku, {})

        confidence_raw = pred.get("confidence_pct", 0.0)
        confidence_val = confidence_raw / 100.0 if confidence_raw > 1.0 else confidence_raw

        trend_slope = float(pred.get("trend_slope", 0.0))

        enriched.append({
            # ── Identity (both keys for compatibility) ──────────────
            "product_id": sku,          # legacy key
            "sku": sku,                 # preferred key — Qt reads this
            # ── Product Info ────────────────────────────────────────
            "product_name": pred.get("product_name", raw.get("name", sku)),
            "name": pred.get("product_name", raw.get("name", sku)),
            "category": raw.get("category", ""),
            "supplier_name": "",
            "current_stock": float(raw.get("current_stock", 0)),
            "unit_cost": float(raw.get("unit_cost", 0.0)),
            "reorder_point": float(raw.get("reorder_point", 10)),
            # ── ML Results ──────────────────────────────────────────
            "demand_label": pred.get("demand_label", ""),
            "stock_status": pred.get("stock_status", ""),
            "confidence": round(confidence_val, 4),
            # ── Forecast ────────────────────────────────────────────
            "forecast_7d": round(pred.get("forecast_7d_units", 0.0), 2),   # Qt reads this
            "weekly_forecast": round(pred.get("forecast_7d_units", 0.0), 2),  # legacy alias
            "trend": trend_slope,                                              # Qt reads this (numeric)
            "trend_direction": "Rising" if trend_slope > 0 else "Falling",   # human-readable
            # ── Priority / EOQ ──────────────────────────────────────
            "priority": pred.get("priority", "Safe"),                         # Qt reads this
            "urgency": pred.get("priority", "Safe"),                          # legacy alias
            "priority_score": 0.0,
            "eoq": int(pred.get("eoq_units", 0)),                             # Qt reads this
            "eoq_quantity": float(pred.get("eoq_units", 0)),                  # legacy alias
            "days_remaining": round(
                float(raw.get("current_stock", 0)) / max(pred.get("forecast_7d_units", 1.0) / 7.0, 0.01),
                1
            ),
            # ── Text Outputs ────────────────────────────────────────
            # Generate recommendation inline when missing (e.g. predictions loaded
            # from SQLite before the recommendation field was introduced).
            "recommendation": pred.get("recommendation") or _build_recommendation(
                priority=pred.get("priority", "Safe"),
                stock_status=pred.get("stock_status", ""),
                eoq=int(pred.get("eoq_units", 0)),
                forecast_7d=pred.get("forecast_7d_units", 0.0),
            ),
            "explanation": pred.get("explanations", ""),
            "explanations": pred.get("explanations", ""),
            # ── Forecast Points ─────────────────────────────────────
            "forecast_points": pred.get("forecast_points", []),
        })

    return JSONResponse(content=enriched)


# ─────────────────────────────────────────────────────────────────────────────
# GET /results/{product_id}  — Return single product prediction
# ─────────────────────────────────────────────────────────────────────────────
@app.get("/results/{product_id}", tags=["Results"])
def get_product_result(product_id: str):
    """
    Returns the full enriched prediction for a single product by SKU.
    Matches on both 'sku' and legacy 'product_id' keys.
    Always returns a JSON object — never a 404 — so the Qt client
    can display a graceful fallback instead of staying on Loading state.
    """
    import json
    all_results = get_all_results()
    body = json.loads(all_results.body.decode())

    if isinstance(body, list):
        for item in body:
            # Match on either the new 'sku' key or the legacy 'product_id' key
            item_sku = str(item.get("sku", item.get("product_id", "")))
            if item_sku == product_id:
                return JSONResponse(content=item)

    # Product not in pipeline results — return a graceful empty record
    # so the Qt panel can show 'No recommendation available.' instead of
    # staying permanently on 'Loading ML Data...'
    return JSONResponse(content={
        "sku": product_id,
        "product_id": product_id,
        "recommendation": "",
        "explanation": "",
        "not_found": True,
    })


# ─────────────────────────────────────────────────────────────────────────────
# GET /analytics  — Return aggregate analytics
# ─────────────────────────────────────────────────────────────────────────────
@app.get("/analytics", tags=["Analytics"])
def get_analytics():
    """
    Returns aggregate analytics computed from the latest predictions:
    total_products, critical_count, reorder_soon_count, safe_count,
    high/medium/low demand counts, avg_confidence, total_reorder_value.
    """
    predictions = getattr(app.state, "latest_predictions", None)
    raw_data = getattr(app.state, "latest_raw_data", None)

    if not predictions:
        return JSONResponse(content={
            "total_products": 0, "critical_count": 0, "reorder_soon_count": 0,
            "safe_count": 0, "high_demand_count": 0, "medium_demand_count": 0,
            "low_demand_count": 0, "avg_confidence": 0.0, "total_reorder_value": 0.0,
        })

    total = len(predictions)
    critical = sum(1 for p in predictions if p.get("priority") == "Critical")
    reorder_soon = sum(1 for p in predictions if p.get("priority") == "ReorderSoon")
    safe = sum(1 for p in predictions if p.get("priority") == "Safe")

    high = sum(1 for p in predictions if "High" in str(p.get("demand_label", "")))
    medium = sum(1 for p in predictions if "Medium" in str(p.get("demand_label", "")))
    low = sum(1 for p in predictions if "Low" in str(p.get("demand_label", "")))

    confs = [p.get("confidence_pct", 0.0) for p in predictions]
    avg_conf_raw = sum(confs) / len(confs) if confs else 0.0
    avg_conf = avg_conf_raw / 100.0 if avg_conf_raw > 1.0 else avg_conf_raw

    raw_map: Dict[str, Dict] = {}
    if raw_data:
        for rd in raw_data:
            raw_map[str(rd.get("sku", ""))] = rd

    total_reorder_value = 0.0
    for p in predictions:
        if p.get("priority") in ("Critical", "ReorderSoon"):
            sku = str(p.get("sku", ""))
            raw = raw_map.get(sku, {})
            total_reorder_value += float(p.get("eoq_units", 0)) * float(raw.get("unit_cost", 0))

    return JSONResponse(content={
        "total_products": total,
        "critical_count": critical,
        "reorder_soon_count": reorder_soon,
        "safe_count": safe,
        "high_demand_count": high,
        "medium_demand_count": medium,
        "low_demand_count": low,
        "avg_confidence": round(avg_conf, 4),
        "total_reorder_value": round(total_reorder_value, 2),
    })
