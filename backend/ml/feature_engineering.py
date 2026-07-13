"""
Feature Engineering Engine for the AI Inventory Management Backend.
Generates statistical, date, lag, and stock features for machine learning pipelines.
Safely handles missing values to prevent NaNs from reaching classifiers.
"""

import pandas as pd
import numpy as np
from typing import Dict, Any
from backend.utils.helpers import get_season

def extract_features_for_product(
    history_df: pd.DataFrame,
    product_info: Dict[str, Any],
    lead_time_days: int = 7,
    ordering_cost: float = 20.0,
    holding_cost_rate: float = 0.25
) -> pd.DataFrame:
    """
    Generates lag and rolling features from daily/weekly sales history for a single product.
    
    Args:
        history_df: DataFrame with 'date' and 'sales_volume' columns.
        product_info: Dict with static values: 'current_stock', 'unit_cost', 'reorder_point'.
        lead_time_days: Configured lead time.
        ordering_cost: Configured ordering cost.
        holding_cost_rate: Configured annual holding cost rate.
        
    Returns:
        pd.DataFrame: Feature matrix (one row per date).
    """
    if history_df.empty:
        return pd.DataFrame()
        
    # Ensure sorted by date
    df = history_df.copy()
    df['date'] = pd.to_datetime(df['date'])
    df = df.sort_values('date').reset_index(drop=True)
    
    # ── 1. Date Components ──
    df['week_number'] = df['date'].dt.isocalendar().week.astype(int)
    df['month'] = df['date'].dt.month
    df['quarter'] = df['date'].dt.quarter
    df['season'] = df['month'].apply(get_season)
    
    # ── 2. Lag Features ──
    for lag in (1, 2, 3, 7):
        df[f'lag_{lag}'] = df['sales_volume'].shift(lag)
        
    # ── 3. Rolling Statistics ──
    for window in (7, 14, 30):
        # Limit window sizes if dataset is small
        actual_window = min(window, len(df))
        if actual_window > 0:
            df[f'rolling_mean_{window}'] = df['sales_volume'].rolling(window=actual_window, min_periods=1).mean()
            df[f'rolling_std_{window}'] = df['sales_volume'].rolling(window=actual_window, min_periods=1).std()
            df[f'rolling_sum_{window}'] = df['sales_volume'].rolling(window=actual_window, min_periods=1).sum()
        else:
            df[f'rolling_mean_{window}'] = 0.0
            df[f'rolling_std_{window}'] = 0.0
            df[f'rolling_sum_{window}'] = 0.0
            
    # ── 4. Exponential Moving Average ──
    df['ema_7'] = df['sales_volume'].ewm(span=7, adjust=False).mean()
    df['ema_30'] = df['sales_volume'].ewm(span=30, adjust=False).mean()
    
    # ── 5. Growth Rate (Percentage change compared to prior day) ──
    df['growth_rate'] = df['sales_volume'].pct_change(fill_method=None).replace([np.inf, -np.inf], 0.0)
    
    # ── 6. Static Product & Inventory Context ──
    df['current_stock'] = int(product_info.get('current_stock', 0))
    df['unit_cost'] = float(product_info.get('unit_cost', 0.0))
    df['reorder_point'] = int(product_info.get('reorder_point', 10))
    df['lead_time'] = int(lead_time_days)
    df['order_cost'] = float(ordering_cost)
    
    # Holding Cost = unit cost * holding cost rate
    df['holding_cost'] = df['unit_cost'] * float(holding_cost_rate)
    
    # ── 7. Average Usage ──
    df['average_usage'] = df['sales_volume'].mean()
    
    # ── 8. Demand Trend (linear slope of past 7 days) ──
    # Calculate simple rolling trend
    def get_trend(x):
        if len(x) < 2:
            return 0.0
        indices = np.arange(len(x))
        slope, _ = np.polyfit(indices, x, 1)
        return float(slope)
        
    df['demand_trend'] = df['sales_volume'].rolling(window=7, min_periods=2).apply(get_trend)
    
    # ── 9. Clean Missing & NaN Values ──
    # Forward fill then backward fill then zero-fill to eliminate NaNs
    df = df.ffill().bfill().fillna(0.0)
    
    return df
