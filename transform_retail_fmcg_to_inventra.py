"""
transform_retail_fmcg_to_inventra.py
=====================================

Reproducible transformation of the Indian FMCG Retail Sales & Customer & Inventory
(2024) dataset into the Inventra ML pipeline training format.

Source:  Indian FMCG Retail Sales  Customer  Inventory (2024).csv
         100,000 transaction-level rows, 21 columns, Jan–Dec 2024

Target:  inventra_training_data.csv
         Weekly product-level observations with exactly 11 columns

Author:  Inventra Data Engineering Pipeline
Date:    2026-09-01

KEY DESIGN DECISIONS (documented per specification):
─────────────────────────────────────────────────────
1. PRODUCT IDENTITY: City + Brand + Category
   - The dataset contains 8 cities × 8 brands × 8 categories = 512 entities
   - Stock_On_Hand varies independently across cities for the same Brand+Category
   - Therefore each city's inventory is a SEPARATE entity
   - product_id format: "{CITY}_{BRAND}_{CATEGORY}" (underscores, uppercase)
   - product_name format: "{Brand} {Category} ({City})"

2. CLOSING STOCK: Last observation per entity per week
   - Stock_On_Hand varies within the same entity-day (multiple transactions)
   - For each entity-week, we take the Stock_On_Hand from the LATEST
     transaction (by timestamp) in that week
   - This is the best temporal approximation of end-of-week inventory

3. UNIT PRICE: Weighted average Selling_Price per entity per week
   - Selling_Price varies per transaction (different quantities, timestamps)
   - We use the revenue-weighted average: SUM(Revenue) / SUM(Units)
   - This preserves actual revenue relationships

4. REORDER POINT: Median per entity per week
   - Reorder_Level varies across transactions (≈60 unique values per entity)
   - This appears to be a parameter that fluctuates or is sampled
   - We take the MEDIAN of observations within each week as the most robust
     central estimate

5. LEAD TIME: Median per entity per week
   - Lead_Time_Days varies across transactions (range: 3–14 days)
   - Same approach as reorder point: weekly median

6. ORDER COST: Fixed 20.0 INR (NOT AVAILABLE IN SOURCE)
   - The source dataset contains no ordering/procurement cost field
   - Using Inventra's documented default: 20.0 INR

7. HOLDING COST: unit_price_inr × 0.25
   - No annual holding cost in source
   - Derived using Inventra's documented convention: 25% of unit cost

8. MISSING WEEKS: Forward-filled closing stock, zero sales
   - If a product-entity has no transactions in a week but was active,
     weekly_sales = 0 and closing_stock is forward-filled from the last
     known observation (stock doesn't vanish between sales events)
"""

import os
import sys
import pandas as pd
import numpy as np
from datetime import datetime, timedelta

# ─────────────────────────────────────────────────────────────────────────────
# CONFIGURATION
# ─────────────────────────────────────────────────────────────────────────────

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

SOURCE_CSV = os.path.join(
    SCRIPT_DIR,
    "Indian FMCG Retail Sales  Customer  Inventory (2024).csv"
)

OUTPUT_TRAINING_CSV = os.path.join(SCRIPT_DIR, "inventra_training_data.csv")
OUTPUT_INSUFFICIENT_CSV = os.path.join(SCRIPT_DIR, "inventra_insufficient_history.csv")
OUTPUT_QUALITY_REPORT = os.path.join(SCRIPT_DIR, "inventra_data_quality_report.txt")

# Inventra defaults for fields not available in source
DEFAULT_ORDER_COST = 20.0      # INR — Inventra pipeline default
HOLDING_COST_RATE = 0.25       # 25% of unit cost — Inventra convention
MIN_WEEKS_FOR_TRAINING = 7     # Inventra pipeline minimum requirement


# ─────────────────────────────────────────────────────────────────────────────
# STEP 1: LOAD AND VALIDATE SOURCE DATA
# ─────────────────────────────────────────────────────────────────────────────

def load_and_validate(filepath: str) -> pd.DataFrame:
    """Load source CSV and validate expected schema."""
    print("=" * 70)
    print("STEP 1: LOADING AND VALIDATING SOURCE DATA")
    print("=" * 70)

    if not os.path.exists(filepath):
        print(f"ERROR: Source file not found: {filepath}")
        sys.exit(1)

    df = pd.read_csv(filepath)
    print(f"  Loaded: {filepath}")
    print(f"  Shape:  {df.shape[0]:,} rows × {df.shape[1]} columns")

    # Validate required columns exist
    required_cols = [
        "Invoice_ID", "Invoice_Date", "City", "Category", "Brand",
        "Units", "Selling_Price", "Revenue", "Stock_On_Hand",
        "Reorder_Level", "Lead_Time_Days"
    ]
    missing = [c for c in required_cols if c not in df.columns]
    if missing:
        print(f"  ERROR: Missing required columns: {missing}")
        sys.exit(1)
    print(f"  Schema validation: PASSED (all {len(required_cols)} required columns present)")

    return df


# ─────────────────────────────────────────────────────────────────────────────
# STEP 2: INSPECT AND REPORT SOURCE DATA
# ─────────────────────────────────────────────────────────────────────────────

def inspect_source(df: pd.DataFrame) -> dict:
    """Generate a comprehensive inspection of the source data."""
    print("\n" + "=" * 70)
    print("STEP 2: SOURCE DATA INSPECTION REPORT")
    print("=" * 70)

    stats = {}

    # Basic shape
    stats["source_rows"] = len(df)
    stats["source_cols"] = len(df.columns)
    print(f"  Rows:    {stats['source_rows']:,}")
    print(f"  Columns: {stats['source_cols']}")

    # Column data types
    print("\n  Column Data Types:")
    for col in df.columns:
        print(f"    {col:20s} → {df[col].dtype}")

    # Missing values
    print("\n  Missing Values:")
    missing = df.isnull().sum()
    stats["missing_values"] = {}
    for col in df.columns:
        count = missing[col]
        pct = count / len(df) * 100
        stats["missing_values"][col] = count
        if count > 0:
            print(f"    {col:20s} → {count:,} ({pct:.1f}%)")
    if missing.sum() == 0:
        print("    None")

    # Duplicate rows
    dup_count = df.duplicated().sum()
    stats["duplicate_rows"] = dup_count
    print(f"\n  Duplicate rows: {dup_count}")

    # Duplicate Invoice_IDs
    dup_invoices = df["Invoice_ID"].duplicated().sum()
    stats["duplicate_invoice_ids"] = dup_invoices
    print(f"  Duplicate Invoice_IDs: {dup_invoices}")

    # Unique values for key categorical columns
    print("\n  Unique Values (Key Columns):")
    for col in ["City", "Store_Format", "Category", "Brand", "Channel", "Payment_Mode"]:
        vals = sorted(df[col].dropna().unique())
        stats[f"unique_{col}"] = len(vals)
        print(f"    {col:15s} → {len(vals):3d} values: {vals}")

    # Date range
    df_dates = pd.to_datetime(df["Invoice_Date"])
    stats["date_min"] = str(df_dates.min())
    stats["date_max"] = str(df_dates.max())
    print(f"\n  Date Range: {stats['date_min']} to {stats['date_max']}")
    print(f"  Date type: Timestamps (datetime with time component)")

    # Numeric column statistics
    print("\n  Numeric Column Statistics:")
    for col in ["Units", "Cost_Price", "Selling_Price", "Revenue",
                 "Stock_On_Hand", "Reorder_Level", "Lead_Time_Days"]:
        desc = df[col].describe()
        stats[f"stats_{col}"] = {
            "min": desc["min"], "max": desc["max"],
            "mean": desc["mean"], "std": desc["std"]
        }
        print(f"    {col:15s} → min={desc['min']:>10.2f}  max={desc['max']:>10.2f}  "
              f"mean={desc['mean']:>10.2f}  std={desc['std']:>8.2f}")

    # Product entity analysis
    df_temp = df.copy()
    df_temp["entity"] = df_temp["City"] + "_" + df_temp["Brand"] + "_" + df_temp["Category"]
    n_products = df_temp["entity"].nunique()
    n_brand_cat = (df_temp["Brand"] + "_" + df_temp["Category"]).nunique()
    stats["unique_products_brand_cat"] = n_brand_cat
    stats["unique_entities_city_brand_cat"] = n_products
    print(f"\n  Product Identity Analysis:")
    print(f"    Brand × Category combos:        {n_brand_cat}")
    print(f"    City × Brand × Category combos: {n_products}")

    # Transactions per entity
    txn_per_entity = df_temp.groupby("entity").size()
    print(f"    Transactions per entity: min={txn_per_entity.min()}, "
          f"max={txn_per_entity.max()}, mean={txn_per_entity.mean():.1f}")

    # Stock_On_Hand analysis
    print(f"\n  Stock_On_Hand Analysis:")
    stock_per_entity = df_temp.groupby("entity")["Stock_On_Hand"].agg(["min", "max", "nunique"])
    print(f"    Varies within entities: YES (mean {stock_per_entity['nunique'].mean():.0f} "
          f"unique values per entity)")
    print(f"    Interpretation: Transaction-specific snapshot (NOT constant per product)")

    # Reorder_Level analysis
    rl_per_entity = df_temp.groupby("entity")["Reorder_Level"].nunique()
    print(f"\n  Reorder_Level Analysis:")
    print(f"    Varies within entities: YES (mean {rl_per_entity.mean():.0f} unique values per entity)")
    print(f"    Interpretation: Fluctuating parameter — will use weekly median")

    # Lead_Time_Days analysis
    lt_per_entity = df_temp.groupby("entity")["Lead_Time_Days"].nunique()
    print(f"\n  Lead_Time_Days Analysis:")
    print(f"    Varies within entities: YES (mean {lt_per_entity.mean():.0f} unique values per entity)")
    print(f"    Interpretation: Fluctuating parameter — will use weekly median")

    # Order cost check
    print(f"\n  Order Cost:")
    print(f"    NOT AVAILABLE IN SOURCE — will use Inventra default: {DEFAULT_ORDER_COST} INR")

    # Holding cost check
    print(f"\n  Holding Cost:")
    print(f"    NOT AVAILABLE IN SOURCE — will derive as unit_price × {HOLDING_COST_RATE}")

    return stats


# ─────────────────────────────────────────────────────────────────────────────
# STEP 3: CLEAN AND PREPARE
# ─────────────────────────────────────────────────────────────────────────────

def clean_and_prepare(df: pd.DataFrame) -> pd.DataFrame:
    """Clean the source data and add derived columns needed for aggregation."""
    print("\n" + "=" * 70)
    print("STEP 3: CLEANING AND PREPARING DATA")
    print("=" * 70)

    df = df.copy()

    # Parse timestamps
    df["datetime"] = pd.to_datetime(df["Invoice_Date"])
    print(f"  Parsed {len(df):,} timestamps")

    # Remove fully duplicate rows (if any)
    before = len(df)
    df = df.drop_duplicates()
    removed = before - len(df)
    print(f"  Removed {removed} fully duplicate rows")

    # Create entity key: City + Brand + Category
    # This is the inventory entity — each represents a separate stock location
    df["entity_key"] = df["City"] + "_" + df["Brand"] + "_" + df["Category"]
    print(f"  Created {df['entity_key'].nunique()} unique inventory entities")

    # Create product_id (stable, uppercase, underscores for spaces)
    df["product_id"] = (
        df["City"].str.upper().str.replace(" ", "_") + "_" +
        df["Brand"].str.upper().str.replace(" ", "_") + "_" +
        df["Category"].str.upper().str.replace(" ", "_")
    )

    # Create product_name: "{Brand} {Category} ({City})"
    df["product_name"] = df["Brand"] + " " + df["Category"] + " (" + df["City"] + ")"

    # Compute Monday-based week start
    # ISO week: Monday = day 0
    df["week_start"] = df["datetime"].dt.to_period("W-SUN").apply(lambda x: x.start_time)
    print(f"  Computed week_start (Monday-based) — {df['week_start'].nunique()} unique weeks")

    # Validate numeric fields
    for col in ["Units", "Selling_Price", "Revenue", "Stock_On_Hand",
                 "Reorder_Level", "Lead_Time_Days"]:
        negatives = (df[col] < 0).sum()
        if negatives > 0:
            print(f"  WARNING: {col} has {negatives} negative values")
        nulls = df[col].isnull().sum()
        if nulls > 0:
            print(f"  WARNING: {col} has {nulls} null values")

    print(f"  Data cleaning: COMPLETE")
    return df


# ─────────────────────────────────────────────────────────────────────────────
# STEP 4: WEEKLY AGGREGATION
# ─────────────────────────────────────────────────────────────────────────────

def aggregate_weekly(df: pd.DataFrame) -> pd.DataFrame:
    """
    Aggregate transaction-level data to weekly product-level observations.

    For each (product_id, week_start):
      - weekly_sales:    SUM(Units)
      - closing_stock:   Stock_On_Hand from the LATEST transaction in the week
      - unit_price_inr:  Revenue-weighted average: SUM(Revenue) / SUM(Units)
      - reorder_point:   MEDIAN(Reorder_Level) rounded to integer
      - lead_time:       MEDIAN(Lead_Time_Days) rounded to integer
      - category:        Mode (most frequent) — should be constant per entity
      - product_name:    From entity definition — constant per entity
    """
    print("\n" + "=" * 70)
    print("STEP 4: WEEKLY AGGREGATION")
    print("=" * 70)

    # Sort by entity and timestamp to ensure latest-observation logic is correct
    df = df.sort_values(["entity_key", "datetime"]).reset_index(drop=True)

    # ── Aggregate: weekly_sales = SUM(Units) ──
    weekly_sales = (
        df.groupby(["product_id", "week_start"])["Units"]
        .sum()
        .reset_index()
        .rename(columns={"Units": "weekly_sales"})
    )

    # ── Aggregate: closing_stock = Stock_On_Hand from LAST transaction in week ──
    # The last transaction by timestamp within each product-week
    last_txn_idx = df.groupby(["product_id", "week_start"])["datetime"].idxmax()
    closing_stock = (
        df.loc[last_txn_idx, ["product_id", "week_start", "Stock_On_Hand"]]
        .rename(columns={"Stock_On_Hand": "closing_stock"})
        .reset_index(drop=True)
    )

    # ── Aggregate: unit_price_inr = SUM(Revenue) / SUM(Units) ──
    revenue_agg = (
        df.groupby(["product_id", "week_start"])
        .agg(total_revenue=("Revenue", "sum"), total_units=("Units", "sum"))
        .reset_index()
    )
    revenue_agg["unit_price_inr"] = revenue_agg["total_revenue"] / revenue_agg["total_units"]
    unit_price = revenue_agg[["product_id", "week_start", "unit_price_inr"]]

    # ── Aggregate: reorder_point = MEDIAN(Reorder_Level) per week ──
    reorder = (
        df.groupby(["product_id", "week_start"])["Reorder_Level"]
        .median()
        .round()
        .astype(int)
        .reset_index()
        .rename(columns={"Reorder_Level": "reorder_point"})
    )

    # ── Aggregate: lead_time = MEDIAN(Lead_Time_Days) per week ──
    lead_time = (
        df.groupby(["product_id", "week_start"])["Lead_Time_Days"]
        .median()
        .round()
        .astype(int)
        .reset_index()
        .rename(columns={"Lead_Time_Days": "lead_time"})
    )

    # ── Static fields: product_name, category (constant per entity) ──
    entity_info = (
        df.groupby("product_id")
        .agg(
            product_name=("product_name", "first"),
            category=("Category", "first"),
        )
        .reset_index()
    )

    # ── Merge all aggregations ──
    weekly = weekly_sales.copy()
    weekly = weekly.merge(closing_stock, on=["product_id", "week_start"], how="left")
    weekly = weekly.merge(unit_price, on=["product_id", "week_start"], how="left")
    weekly = weekly.merge(reorder, on=["product_id", "week_start"], how="left")
    weekly = weekly.merge(lead_time, on=["product_id", "week_start"], how="left")
    weekly = weekly.merge(entity_info, on="product_id", how="left")

    print(f"  Aggregated to {len(weekly):,} product-week observations")
    print(f"  Unique products: {weekly['product_id'].nunique()}")
    print(f"  Week range: {weekly['week_start'].min()} to {weekly['week_start'].max()}")

    return weekly


# ─────────────────────────────────────────────────────────────────────────────
# STEP 5: FILL MISSING WEEKS
# ─────────────────────────────────────────────────────────────────────────────

def fill_missing_weeks(weekly: pd.DataFrame) -> pd.DataFrame:
    """
    For each product, ensure a continuous weekly timeline between its first
    and last observed week.

    Missing weeks get:
      - weekly_sales = 0  (no transactions occurred)
      - closing_stock = forward-filled from last known observation
        (inventory persists between sales events)
      - unit_price_inr = forward-filled (price doesn't change without a transaction)
      - reorder_point = forward-filled
      - lead_time = forward-filled
      - product_name, category = filled from entity info
    """
    print("\n" + "=" * 70)
    print("STEP 5: FILLING MISSING WEEKS")
    print("=" * 70)

    before_rows = len(weekly)

    filled_dfs = []
    products = weekly["product_id"].unique()
    fill_count = 0

    for pid in products:
        prod_df = weekly[weekly["product_id"] == pid].copy()
        prod_df = prod_df.sort_values("week_start")

        # Get this product's ACTUAL observed range
        first_week = prod_df["week_start"].min()
        last_week = prod_df["week_start"].max()

        # Create full weekly index ONLY within this product's observed range
        product_weeks = pd.date_range(start=first_week, end=last_week, freq="W-MON")
        # If W-MON doesn't align, use the product's actual first week as anchor
        if len(product_weeks) == 0 or product_weeks[0] != first_week:
            product_weeks = pd.date_range(start=first_week, end=last_week, freq="7D")

        # Set week_start as index and reindex to fill gaps
        prod_df = prod_df.set_index("week_start")
        prod_df = prod_df.reindex(product_weeks)

        # Count how many weeks were added
        missing_weeks = prod_df["product_id"].isnull().sum()
        fill_count += missing_weeks

        # Fill product identity (constant per product)
        prod_df["product_id"] = pid
        prod_df["product_name"] = prod_df["product_name"].ffill().bfill()
        prod_df["category"] = prod_df["category"].ffill().bfill()

        # Fill weekly_sales with 0 for missing weeks (no sales occurred)
        prod_df["weekly_sales"] = prod_df["weekly_sales"].fillna(0).astype(int)

        # Forward-fill closing stock (inventory persists between events)
        prod_df["closing_stock"] = prod_df["closing_stock"].ffill().bfill()
        # Safely convert to int (handle any remaining NaN)
        prod_df["closing_stock"] = pd.to_numeric(prod_df["closing_stock"], errors="coerce").fillna(0).astype(int)

        # Forward-fill price, reorder_point, lead_time
        for col in ["unit_price_inr", "reorder_point", "lead_time"]:
            prod_df[col] = prod_df[col].ffill().bfill()
            prod_df[col] = pd.to_numeric(prod_df[col], errors="coerce").fillna(0)

        prod_df = prod_df.reset_index().rename(columns={"index": "week_start"})
        filled_dfs.append(prod_df)

    weekly_filled = pd.concat(filled_dfs, ignore_index=True)

    print(f"  Before filling: {before_rows:,} rows")
    print(f"  Weeks added:    {fill_count:,} (zero-sales weeks)")
    print(f"  After filling:  {len(weekly_filled):,} rows")

    return weekly_filled


# ─────────────────────────────────────────────────────────────────────────────
# STEP 6: COMPUTE DERIVED FIELDS AND FORMAT OUTPUT
# ─────────────────────────────────────────────────────────────────────────────

def format_output(weekly: pd.DataFrame) -> pd.DataFrame:
    """
    Compute derived fields (order_cost, holding_cost) and format the
    final DataFrame to match the exact Inventra target schema.
    """
    print("\n" + "=" * 70)
    print("STEP 6: COMPUTING DERIVED FIELDS AND FORMATTING OUTPUT")
    print("=" * 70)

    df = weekly.copy()

    # Format week_start_date as YYYY-MM-DD string
    df["week_start_date"] = pd.to_datetime(df["week_start"]).dt.strftime("%Y-%m-%d")

    # Order cost: NOT AVAILABLE IN SOURCE — using Inventra default
    df["order_cost"] = DEFAULT_ORDER_COST
    print(f"  order_cost: Set to {DEFAULT_ORDER_COST} INR (NOT AVAILABLE IN SOURCE)")

    # Holding cost: unit_price_inr × 0.25
    df["holding_cost"] = (df["unit_price_inr"] * HOLDING_COST_RATE).round(4)
    print(f"  holding_cost: Derived as unit_price_inr × {HOLDING_COST_RATE}")

    # Ensure integer types where needed
    df["weekly_sales"] = df["weekly_sales"].astype(int)
    df["closing_stock"] = df["closing_stock"].astype(int)
    df["reorder_point"] = df["reorder_point"].astype(int)
    df["lead_time"] = df["lead_time"].astype(int)

    # Round float fields
    df["unit_price_inr"] = df["unit_price_inr"].round(4)
    df["order_cost"] = df["order_cost"].round(4)

    # Select and order columns per Inventra target schema
    output_cols = [
        "product_id",
        "product_name",
        "category",
        "week_start_date",
        "weekly_sales",
        "closing_stock",
        "unit_price_inr",
        "reorder_point",
        "lead_time",
        "order_cost",
        "holding_cost",
    ]

    df = df[output_cols].copy()

    # Sort: product_id ASC, week_start_date ASC (temporal order)
    df = df.sort_values(["product_id", "week_start_date"]).reset_index(drop=True)

    print(f"  Output schema: {list(df.columns)}")
    print(f"  Total rows: {len(df):,}")

    return df


# ─────────────────────────────────────────────────────────────────────────────
# STEP 7: DATA QUALITY VALIDATION
# ─────────────────────────────────────────────────────────────────────────────

def validate_output(df: pd.DataFrame) -> dict:
    """Run comprehensive data quality checks on the final dataset."""
    print("\n" + "=" * 70)
    print("STEP 7: DATA QUALITY VALIDATION")
    print("=" * 70)

    checks = {}
    all_passed = True

    # ── Schema check ──
    expected_cols = [
        "product_id", "product_name", "category", "week_start_date",
        "weekly_sales", "closing_stock", "unit_price_inr", "reorder_point",
        "lead_time", "order_cost", "holding_cost"
    ]
    schema_ok = list(df.columns) == expected_cols
    checks["schema_valid"] = schema_ok
    print(f"  Schema check:         {'PASS' if schema_ok else 'FAIL'}")
    if not schema_ok:
        all_passed = False

    # ── Null product IDs ──
    null_pids = df["product_id"].isnull().sum()
    checks["null_product_ids"] = null_pids
    passed = null_pids == 0
    print(f"  No null product_ids:  {'PASS' if passed else 'FAIL'} ({null_pids})")
    if not passed:
        all_passed = False

    # ── Duplicate product-week rows ──
    dup_pw = df.duplicated(subset=["product_id", "week_start_date"]).sum()
    checks["duplicate_product_weeks"] = dup_pw
    passed = dup_pw == 0
    print(f"  No duplicate prod-wk: {'PASS' if passed else 'FAIL'} ({dup_pw})")
    if not passed:
        all_passed = False

    # ── Valid dates ──
    try:
        dates = pd.to_datetime(df["week_start_date"])
        valid_dates = True
        # Check Monday start
        non_monday = (dates.dt.dayofweek != 0).sum()
        checks["non_monday_dates"] = non_monday
        if non_monday > 0:
            print(f"  Monday week start:    FAIL ({non_monday} non-Monday dates)")
            valid_dates = False
        else:
            print(f"  Monday week start:    PASS")
    except Exception:
        valid_dates = False
        print(f"  Date parsing:         FAIL")
    checks["valid_dates"] = valid_dates
    if not valid_dates:
        all_passed = False

    # ── Numeric, non-negative checks ──
    for col, must_be_positive in [
        ("weekly_sales", False),      # Can be 0
        ("closing_stock", False),     # Can be 0
        ("unit_price_inr", True),     # Must be > 0
        ("reorder_point", False),     # Can be 0
        ("lead_time", False),         # Can be 0
        ("order_cost", False),        # Can be 0
        ("holding_cost", False),      # Can be 0
    ]:
        negatives = (df[col] < 0).sum()
        nulls = df[col].isnull().sum()
        zeros = (df[col] == 0).sum() if must_be_positive else 0
        passed = negatives == 0 and nulls == 0 and zeros == 0
        checks[f"valid_{col}"] = passed
        status = "PASS" if passed else "FAIL"
        details = []
        if negatives > 0:
            details.append(f"{negatives} negatives")
        if nulls > 0:
            details.append(f"{nulls} nulls")
        if zeros > 0 and must_be_positive:
            details.append(f"{zeros} zeros")
        detail_str = f" ({', '.join(details)})" if details else ""
        print(f"  {col:20s}: {status}{detail_str}")
        if not passed:
            all_passed = False

    # ── Temporal integrity: no future leakage ──
    # Check that within each product, weeks are in chronological order
    temporal_ok = True
    for pid in df["product_id"].unique():
        prod_dates = pd.to_datetime(df[df["product_id"] == pid]["week_start_date"])
        if not prod_dates.is_monotonic_increasing:
            temporal_ok = False
            break
    checks["temporal_integrity"] = temporal_ok
    print(f"  Temporal integrity:   {'PASS' if temporal_ok else 'FAIL'}")
    if not temporal_ok:
        all_passed = False

    checks["all_passed"] = all_passed
    print(f"\n  OVERALL VALIDATION:   {'✅ ALL CHECKS PASSED' if all_passed else '❌ SOME CHECKS FAILED'}")

    return checks


# ─────────────────────────────────────────────────────────────────────────────
# STEP 8: SPLIT AND SAVE
# ─────────────────────────────────────────────────────────────────────────────

def split_and_save(df: pd.DataFrame) -> dict:
    """
    Split products by history length and save output files.
    Products with < MIN_WEEKS_FOR_TRAINING weeks go to insufficient_history.csv
    """
    print("\n" + "=" * 70)
    print("STEP 8: SPLITTING AND SAVING OUTPUT FILES")
    print("=" * 70)

    # Count weeks per product
    weeks_per_product = df.groupby("product_id")["week_start_date"].nunique()

    sufficient = weeks_per_product[weeks_per_product >= MIN_WEEKS_FOR_TRAINING].index
    insufficient = weeks_per_product[weeks_per_product < MIN_WEEKS_FOR_TRAINING].index

    df_sufficient = df[df["product_id"].isin(sufficient)].copy()
    df_insufficient = df[df["product_id"].isin(insufficient)].copy()

    # Save main training data
    df_sufficient.to_csv(OUTPUT_TRAINING_CSV, index=False)
    print(f"  Saved: {OUTPUT_TRAINING_CSV}")
    print(f"    → {len(df_sufficient):,} rows, {len(sufficient)} products")

    # Save insufficient history (if any)
    if len(df_insufficient) > 0:
        df_insufficient.to_csv(OUTPUT_INSUFFICIENT_CSV, index=False)
        print(f"  Saved: {OUTPUT_INSUFFICIENT_CSV}")
        print(f"    → {len(df_insufficient):,} rows, {len(insufficient)} products")
    else:
        print(f"  No products with insufficient history (all have ≥ {MIN_WEEKS_FOR_TRAINING} weeks)")

    stats = {
        "sufficient_products": len(sufficient),
        "insufficient_products": len(insufficient),
        "sufficient_rows": len(df_sufficient),
        "insufficient_rows": len(df_insufficient),
        "weeks_per_product": weeks_per_product,
    }

    return stats


# ─────────────────────────────────────────────────────────────────────────────
# STEP 9: GENERATE QUALITY REPORT
# ─────────────────────────────────────────────────────────────────────────────

def generate_quality_report(
    df: pd.DataFrame,
    source_stats: dict,
    split_stats: dict,
    validation: dict,
) -> None:
    """Generate and save the comprehensive data quality report."""
    print("\n" + "=" * 70)
    print("STEP 9: GENERATING DATA QUALITY REPORT")
    print("=" * 70)

    weeks_per_product = split_stats["weeks_per_product"]

    lines = []
    lines.append("=" * 70)
    lines.append("INVENTRA DATA QUALITY REPORT")
    lines.append("=" * 70)
    lines.append(f"Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    lines.append(f"Source:     Indian FMCG Retail Sales  Customer  Inventory (2024).csv")
    lines.append(f"Output:     inventra_training_data.csv")
    lines.append("")

    # ── Source Dataset ──
    lines.append("─" * 40)
    lines.append("SOURCE DATASET")
    lines.append("─" * 40)
    lines.append(f"Original row count:      {source_stats['source_rows']:,}")
    lines.append(f"Original column count:   {source_stats['source_cols']}")
    lines.append(f"Date range:              {source_stats['date_min']} to {source_stats['date_max']}")
    lines.append(f"Unique cities:           {source_stats['unique_City']}")
    lines.append(f"Unique brands:           {source_stats.get('unique_Brand', 'N/A')}")
    lines.append(f"Unique categories:       {source_stats['unique_Category']}")
    lines.append(f"Unique store formats:    {source_stats['unique_Store_Format']}")
    lines.append(f"Brand×Category combos:   {source_stats['unique_products_brand_cat']}")
    lines.append(f"City×Brand×Cat entities: {source_stats['unique_entities_city_brand_cat']}")
    lines.append("")

    # Missing values in source
    lines.append("Source Missing Values:")
    for col, count in source_stats["missing_values"].items():
        if count > 0:
            pct = count / source_stats["source_rows"] * 100
            lines.append(f"  {col:20s}: {count:,} ({pct:.1f}%)")
    lines.append(f"  Duplicate rows:        {source_stats['duplicate_rows']}")
    lines.append(f"  Duplicate Invoice_IDs: {source_stats['duplicate_invoice_ids']}")
    lines.append("")

    # ── Output Dataset ──
    lines.append("─" * 40)
    lines.append("OUTPUT DATASET")
    lines.append("─" * 40)
    lines.append(f"Final row count:         {len(df):,}")
    lines.append(f"Unique products:         {df['product_id'].nunique()}")
    lines.append(f"Categories:              {df['category'].nunique()}")
    dates = pd.to_datetime(df["week_start_date"])
    lines.append(f"Date range:              {dates.min().strftime('%Y-%m-%d')} to {dates.max().strftime('%Y-%m-%d')}")
    lines.append(f"Total weeks:             {df['week_start_date'].nunique()}")
    lines.append("")

    # Product history distribution
    lines.append("Product History Distribution:")
    lines.append(f"  Products total:        {len(weeks_per_product)}")
    lines.append(f"  Products < 7 weeks:    {(weeks_per_product < 7).sum()}")
    lines.append(f"  Products >= 7 weeks:   {(weeks_per_product >= 7).sum()}")
    lines.append(f"  Products >= 26 weeks:  {(weeks_per_product >= 26).sum()}")
    lines.append(f"  Products >= 50 weeks:  {(weeks_per_product >= 50).sum()}")
    lines.append(f"  Min weeks/product:     {weeks_per_product.min()}")
    lines.append(f"  Max weeks/product:     {weeks_per_product.max()}")
    lines.append(f"  Mean weeks/product:    {weeks_per_product.mean():.1f}")
    lines.append("")

    # Missing values in output
    lines.append("Output Missing Values:")
    total_missing = df.isnull().sum().sum()
    if total_missing == 0:
        lines.append("  None")
    else:
        for col in df.columns:
            count = df[col].isnull().sum()
            if count > 0:
                lines.append(f"  {col:20s}: {count:,}")
    lines.append(f"  Duplicate product-week rows: {validation.get('duplicate_product_weeks', 0)}")
    lines.append("")

    # ── Statistics ──
    lines.append("─" * 40)
    lines.append("FIELD STATISTICS")
    lines.append("─" * 40)

    for col, label in [
        ("weekly_sales", "Weekly Sales"),
        ("closing_stock", "Closing Stock"),
        ("unit_price_inr", "Unit Price (INR)"),
        ("reorder_point", "Reorder Point"),
        ("lead_time", "Lead Time (days)"),
        ("order_cost", "Order Cost (INR)"),
        ("holding_cost", "Holding Cost (INR)"),
    ]:
        desc = df[col].describe()
        lines.append(f"\n{label}:")
        lines.append(f"  min:   {desc['min']:>12.2f}")
        lines.append(f"  max:   {desc['max']:>12.2f}")
        lines.append(f"  mean:  {desc['mean']:>12.2f}")
        lines.append(f"  std:   {desc['std']:>12.2f}")
        lines.append(f"  median:{desc['50%']:>12.2f}")

    # ── Assumptions ──
    lines.append("")
    lines.append("─" * 40)
    lines.append("ASSUMPTIONS AND DECISIONS")
    lines.append("─" * 40)
    lines.append("1. PRODUCT IDENTITY: City + Brand + Category treated as separate")
    lines.append("   inventory entities because Stock_On_Hand varies independently")
    lines.append("   across cities for the same Brand+Category combination.")
    lines.append("")
    lines.append("2. CLOSING STOCK: Taken from the LAST transaction (by timestamp)")
    lines.append("   within each product-week. Stock_On_Hand varies across")
    lines.append("   transactions even on the same day.")
    lines.append("")
    lines.append("3. UNIT PRICE: Revenue-weighted average per product-week:")
    lines.append("   SUM(Revenue) / SUM(Units). Preserves true average revenue.")
    lines.append("")
    lines.append("4. REORDER POINT: Weekly MEDIAN of Reorder_Level observations.")
    lines.append("   The source value varies per transaction (~60 unique values")
    lines.append("   per entity), so median provides a robust central estimate.")
    lines.append("")
    lines.append("5. LEAD TIME: Weekly MEDIAN of Lead_Time_Days observations.")
    lines.append("   Same rationale as reorder point.")
    lines.append("")
    lines.append("6. ORDER COST: NOT AVAILABLE IN SOURCE.")
    lines.append(f"   Fixed at {DEFAULT_ORDER_COST} INR per Inventra pipeline default.")
    lines.append("")
    lines.append("7. HOLDING COST: NOT AVAILABLE IN SOURCE.")
    lines.append(f"   Derived as unit_price_inr × {HOLDING_COST_RATE} per Inventra convention.")
    lines.append("")
    lines.append("8. MISSING WEEKS: For weeks with no transactions, weekly_sales = 0")
    lines.append("   and closing_stock is forward-filled from last known observation.")
    lines.append("   This is justified because inventory persists between sales events.")
    lines.append("")
    lines.append("9. WEEK START: Monday-based (ISO standard). All week_start_date values")
    lines.append("   are the Monday of the corresponding week.")
    lines.append("")

    # ── Validation Summary ──
    lines.append("─" * 40)
    lines.append("VALIDATION SUMMARY")
    lines.append("─" * 40)
    lines.append(f"Schema valid:              {'YES' if validation.get('schema_valid') else 'NO'}")
    lines.append(f"Temporal integrity valid:  {'YES' if validation.get('temporal_integrity') else 'NO'}")
    lines.append(f"Data quality valid:        {'YES' if validation.get('all_passed') else 'NO'}")
    lines.append("")

    # ── Fields Not Available ──
    lines.append("─" * 40)
    lines.append("FIELDS NOT AVAILABLE IN SOURCE DATASET")
    lines.append("─" * 40)
    lines.append("order_cost:   No ordering/procurement cost column exists in source.")
    lines.append(f"              Used Inventra default: {DEFAULT_ORDER_COST} INR")
    lines.append("holding_cost: No annual holding cost column exists in source.")
    lines.append(f"              Derived as unit_price_inr × {HOLDING_COST_RATE}")
    lines.append("")

    report_text = "\n".join(lines)

    with open(OUTPUT_QUALITY_REPORT, "w") as f:
        f.write(report_text)

    print(f"  Saved: {OUTPUT_QUALITY_REPORT}")
    print(f"  Report length: {len(lines)} lines")


# ─────────────────────────────────────────────────────────────────────────────
# STEP 10: FINAL VALIDATION SUMMARY
# ─────────────────────────────────────────────────────────────────────────────

def print_final_validation(df: pd.DataFrame, split_stats: dict, validation: dict):
    """Print the standardized final validation output."""
    weeks_per_product = split_stats["weeks_per_product"]
    dates = pd.to_datetime(df["week_start_date"])

    print("\n")
    print("INVENTRA DATASET VALIDATION")
    print("===========================")
    print()
    print(f"Rows:             {len(df):,}")
    print(f"Unique products:  {df['product_id'].nunique()}")
    print(f"Categories:       {df['category'].nunique()}")
    print(f"Date range:       {dates.min().strftime('%Y-%m-%d')} to {dates.max().strftime('%Y-%m-%d')}")
    print()
    print(f"Products >= 7 weeks:  {(weeks_per_product >= 7).sum()}")
    print(f"Products >= 50 weeks: {(weeks_per_product >= 50).sum()}")
    print()
    print(f"Missing values:           {df.isnull().sum().sum()}")
    print(f"Duplicate product-week rows: {validation.get('duplicate_product_weeks', 0)}")
    print()

    for col, label in [
        ("weekly_sales", "Weekly sales"),
        ("closing_stock", "Closing stock"),
        ("unit_price_inr", "Unit price"),
        ("reorder_point", "Reorder point"),
        ("lead_time", "Lead time"),
    ]:
        desc = df[col].describe()
        print(f"{label}:")
        print(f"  min:  {desc['min']:>10.2f}")
        print(f"  max:  {desc['max']:>10.2f}")
        print(f"  mean: {desc['mean']:>10.2f}")
        print()

    print(f"Schema valid:              {'YES' if validation.get('schema_valid') else 'NO'}")
    print(f"Temporal integrity valid:  {'YES' if validation.get('temporal_integrity') else 'NO'}")
    print(f"Data quality valid:        {'YES' if validation.get('all_passed') else 'NO'}")
    print()
    if validation.get("all_passed"):
        print("✅ Dataset is ready for Inventra ML pipeline training.")
    else:
        print("❌ Some validation checks failed. Review the quality report.")


# ─────────────────────────────────────────────────────────────────────────────
# MAIN PIPELINE
# ─────────────────────────────────────────────────────────────────────────────

def main():
    print("\n" + "█" * 70)
    print("  INVENTRA DATA TRANSFORMATION PIPELINE")
    print("  Source: Indian FMCG Retail Sales & Inventory (2024)")
    print("  Target: Inventra ML Training Format")
    print("█" * 70)

    # Step 1: Load and validate source
    df_raw = load_and_validate(SOURCE_CSV)

    # Step 2: Inspect source data
    source_stats = inspect_source(df_raw)

    # Step 3: Clean and prepare
    df_clean = clean_and_prepare(df_raw)

    # Step 4: Weekly aggregation
    df_weekly = aggregate_weekly(df_clean)

    # Step 5: Fill missing weeks
    df_filled = fill_missing_weeks(df_weekly)

    # Step 6: Compute derived fields and format output
    df_output = format_output(df_filled)

    # Step 7: Validate output
    validation = validate_output(df_output)

    # Step 8: Split and save
    split_stats = split_and_save(df_output)

    # Step 9: Generate quality report
    generate_quality_report(df_output, source_stats, split_stats, validation)

    # Step 10: Final validation summary
    print_final_validation(df_output, split_stats, validation)


if __name__ == "__main__":
    main()
