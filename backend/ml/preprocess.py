"""
Preprocessing Module — Inventra FastAPI Backend.

This module is the single source of truth for all data transformations
that must happen BEFORE the SVC classifier sees any data.

Separating it here means:
  - train_model.py can import it to build training features
  - main.py (FastAPI) can import the same function to preprocess
    uploaded CSVs at prediction time
  — both paths are IDENTICAL, so there is no train/serve skew.

CSV → DataFrame column mapping used by the pipeline:
  CSV column          internal name used by ML code
  ─────────────────────────────────────────────────
  product_id          sku  (treated as string SKU)
  product_name        name
  week_start_date     date
  weekly_sales        sales_volume
  closing_stock       current_stock
  unit_price_inr      unit_cost
  reorder_point       reorder_point  (same name)
  lead_time           lead_time      (same name)
  order_cost          order_cost     (same name)
  holding_cost        holding_cost   (same name)
"""

import pandas as pd
import numpy as np
from typing import Dict, Any, List, Tuple

# ── Column rename map ────────────────────────────────────────────────────────
# Maps raw CSV columns to the internal names the ML pipeline expects.
CSV_TO_INTERNAL: Dict[str, str] = {
    "product_id":       "sku",
    "product_name":     "name",
    "week_start_date":  "date",
    "weekly_sales":     "sales_volume",
    "closing_stock":    "current_stock",
    "unit_price_inr":   "unit_cost",
}

# Minimum set of columns that MUST exist in the uploaded CSV
REQUIRED_CSV_COLUMNS: List[str] = [
    "product_id",
    "product_name",
    "week_start_date",
    "weekly_sales",
    "closing_stock",
    "unit_price_inr",
    "reorder_point",
    "lead_time",
    "order_cost",
    "holding_cost",
]

# The 22 feature columns the SVC model was trained on (from model_metadata.json)
CLASSIFIER_FEATURES: List[str] = [
    "sales_volume", "lag_1", "lag_2", "lag_3", "lag_7",
    "rolling_mean_7", "rolling_std_7", "rolling_sum_7",
    "rolling_mean_30", "rolling_std_30", "rolling_sum_30",
    "ema_7", "ema_30", "growth_rate", "current_stock",
    "unit_cost", "reorder_point", "lead_time", "order_cost",
    "holding_cost", "average_usage", "demand_trend",
]


def validate_csv_columns(df: pd.DataFrame) -> Tuple[bool, str]:
    """
    Checks that every required column exists in the uploaded DataFrame.

    Returns:
        (True, "") if valid.
        (False, error_message) if columns are missing.
    """
    missing = [c for c in REQUIRED_CSV_COLUMNS if c not in df.columns]
    if missing:
        return False, f"Missing required columns: {missing}"
    return True, ""


def normalize_csv(df: pd.DataFrame) -> pd.DataFrame:
    """
    Step 1 — Rename raw CSV columns to internal ML names.
    Only renames columns that exist; leaves everything else untouched.
    """
    existing_renames = {k: v for k, v in CSV_TO_INTERNAL.items() if k in df.columns}
    return df.rename(columns=existing_renames)


def _get_season(month: int) -> int:
    """Returns a numeric season code (1=Spring, 2=Summer, 3=Autumn, 4=Winter)."""
    if month in (3, 4, 5):
        return 1
    elif month in (6, 7, 8):
        return 2
    elif month in (9, 10, 11):
        return 3
    return 4


def build_features_for_sku(
    sku_df: pd.DataFrame,
    product_info: Dict[str, Any],
    lead_time_days: int = 7,
    ordering_cost: float = 20.0,
    holding_cost_rate: float = 0.25,
) -> pd.DataFrame:
    """
    Generates all 22 classifier feature columns for a single SKU's time-series.

    This replicates the logic in backend/ml/feature_engineering.py exactly,
    so training and serving use identical transformations.

    Args:
        sku_df: DataFrame with 'date' and 'sales_volume' columns for one SKU.
        product_info: Dict containing current_stock, unit_cost, reorder_point, etc.
        lead_time_days: Supplier lead time in days.
        ordering_cost: Fixed cost per order.
        holding_cost_rate: Annual holding cost as fraction of unit cost.

    Returns:
        pd.DataFrame with all 22 classifier features (one row per date).
    """
    if sku_df.empty:
        return pd.DataFrame()

    df = sku_df.copy()
    df["date"] = pd.to_datetime(df["date"])
    df = df.sort_values("date").reset_index(drop=True)

    # ── Lag Features ──────────────────────────────────────────────────────────
    for lag in (1, 2, 3, 7):
        df[f"lag_{lag}"] = df["sales_volume"].shift(lag)

    # ── Rolling Statistics (windows 7, 14, 30) ────────────────────────────────
    for window in (7, 14, 30):
        actual_window = min(window, len(df))
        if actual_window > 0:
            df[f"rolling_mean_{window}"] = (
                df["sales_volume"].rolling(window=actual_window, min_periods=1).mean()
            )
            df[f"rolling_std_{window}"] = (
                df["sales_volume"].rolling(window=actual_window, min_periods=1).std()
            )
            df[f"rolling_sum_{window}"] = (
                df["sales_volume"].rolling(window=actual_window, min_periods=1).sum()
            )
        else:
            df[f"rolling_mean_{window}"] = 0.0
            df[f"rolling_std_{window}"] = 0.0
            df[f"rolling_sum_{window}"] = 0.0

    # ── Exponential Moving Averages ───────────────────────────────────────────
    df["ema_7"] = df["sales_volume"].ewm(span=7, adjust=False).mean()
    df["ema_30"] = df["sales_volume"].ewm(span=30, adjust=False).mean()

    # ── Growth Rate ───────────────────────────────────────────────────────────
    df["growth_rate"] = (
        df["sales_volume"]
        .pct_change(fill_method=None)
        .replace([np.inf, -np.inf], 0.0)
    )

    # ── Static Product Context ────────────────────────────────────────────────
    df["current_stock"] = int(product_info.get("current_stock", 0))
    df["unit_cost"] = float(product_info.get("unit_cost", 0.0))
    df["reorder_point"] = int(product_info.get("reorder_point", 10))
    df["lead_time"] = int(lead_time_days)
    df["order_cost"] = float(ordering_cost)
    df["holding_cost"] = df["unit_cost"] * float(holding_cost_rate)

    # ── Average Usage & Demand Trend ─────────────────────────────────────────
    df["average_usage"] = df["sales_volume"].mean()

    def _get_trend(x: pd.Series) -> float:
        if len(x) < 2:
            return 0.0
        indices = np.arange(len(x))
        slope, _ = np.polyfit(indices, x, 1)
        return float(slope)

    df["demand_trend"] = df["sales_volume"].rolling(window=7, min_periods=2).apply(
        _get_trend
    )

    # ── Fill any NaNs ─────────────────────────────────────────────────────────
    df = df.ffill().bfill().fillna(0.0)

    return df


def preprocess_dataframe(
    df: pd.DataFrame,
    lead_time_days: int = 7,
    ordering_cost: float = 20.0,
    holding_cost_rate: float = 0.25,
) -> Tuple[pd.DataFrame, List[Dict[str, Any]]]:
    """
    Full preprocessing pipeline for an uploaded CSV.

    Steps:
      1. Rename CSV columns to internal names.
      2. Build feature matrix per SKU (same steps as training).
      3. Extract product metadata list.

    Args:
        df: Raw DataFrame from the uploaded CSV (columns already validated).
        lead_time_days: Default lead time if not in CSV.
        ordering_cost: Default ordering cost if not in CSV.
        holding_cost_rate: Annual holding cost rate.

    Returns:
        Tuple of:
          - features_df: Combined feature matrix for all SKUs (used by classifier)
          - products_list: List of product metadata dicts (used by pipeline coordinator)
    """
    # Step 1: Normalize column names
    df = normalize_csv(df)

    # Step 2: Ensure sales_volume is numeric
    df["sales_volume"] = pd.to_numeric(df["sales_volume"], errors="coerce").fillna(0.0)

    # Step 3: Build features per SKU and collect product metadata
    all_feature_dfs: List[pd.DataFrame] = []
    products_list: List[Dict[str, Any]] = []

    for sku, sku_df in df.groupby("sku"):
        last_row = sku_df.iloc[-1]

        product_info = {
            "id": str(sku),
            "sku": str(sku),
            "name": str(last_row.get("name", sku)),
            "category": str(last_row.get("category", "")),
            "current_stock": int(float(last_row.get("current_stock", 0))),
            "unit_cost": float(last_row.get("unit_cost", 0.0)),
            "reorder_point": int(float(last_row.get("reorder_point", 10))),
        }
        products_list.append(product_info)

        # Use CSV-provided lead_time / order_cost if available per row
        row_lead_time = int(float(last_row.get("lead_time", lead_time_days)))
        row_order_cost = float(last_row.get("order_cost", ordering_cost))
        row_holding_cost_rate = (
            float(last_row.get("holding_cost", product_info["unit_cost"] * holding_cost_rate))
            / max(product_info["unit_cost"], 1e-9)
        )

        feat_df = build_features_for_sku(
            sku_df=sku_df,
            product_info=product_info,
            lead_time_days=row_lead_time,
            ordering_cost=row_order_cost,
            holding_cost_rate=row_holding_cost_rate,
        )

        if not feat_df.empty:
            all_feature_dfs.append(feat_df)

    features_df = (
        pd.concat(all_feature_dfs, ignore_index=True) if all_feature_dfs else pd.DataFrame()
    )

    return features_df, products_list
