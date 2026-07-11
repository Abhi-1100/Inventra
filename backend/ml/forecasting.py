"""
Forecasting Engine for the AI Inventory Management Backend.
Predicts product sales demand for the next forecast horizon.
Uses a tiered model structure: Prophet -> Linear Regression -> Moving Average fallback.
Ensures zero runtime crashes via graceful fallbacks.
"""

import pandas as pd
import numpy as np
from datetime import datetime, timedelta
from typing import Tuple, List, Dict, Any, Optional
from sklearn.linear_model import LinearRegression

from backend.utils.logger import get_logger

logger = get_logger("forecasting")

def forecast_demand(
    sales_df: pd.DataFrame,
    horizon: int = 7,
    start_date: Optional[datetime] = None
) -> Tuple[float, float, List[Dict[str, Any]], str]:
    """
    Predicts sales demand for the next 'horizon' days.
    
    Args:
        sales_df: DataFrame with 'date' (or 'ds') and 'sales_volume' (or 'y') columns.
        horizon: Prediction horizon in days.
        start_date: Start date for predictions. Defaults to datetime.now().
        
    Returns:
        Tuple containing:
            - forecast_total (float): Sum of forecasted sales for the horizon.
            - trend_slope (float): The trend gradient/slope (units/day).
            - forecast_points (list): Daily forecasts (date, yhat, yhat_lower, yhat_upper).
            - model_used (str): Name of the model that generated the prediction.
    """
    if start_date is None:
        start_date = datetime.now()
        
    # Prepare standard format
    df = sales_df.copy()
    if 'date' in df.columns:
        df['ds'] = pd.to_datetime(df['date'])
    else:
        df['ds'] = pd.to_datetime(df['ds'])
        
    if 'sales_volume' in df.columns:
        df['y'] = df['sales_volume'].astype(float)
    else:
        df['y'] = df['y'].astype(float)
        
    # Group by date to handle duplicates
    daily_sales = df.groupby('ds')['y'].sum().reset_index()
    daily_sales = daily_sales.sort_values('ds').reset_index(drop=True)
    
    # ── Try Prophet ──
    try:
        from prophet import Prophet
        import logging
        # Mute Prophet's stdout/stderr logging noise
        logging.getLogger('prophet').setLevel(logging.ERROR)
        
        # Prophet requires at least 2 non-NaN rows to fit
        if len(daily_sales) >= 10:
            span_days = (daily_sales['ds'].max() - daily_sales['ds'].min()).days
            
            m = Prophet(
                yearly_seasonality=False,
                weekly_seasonality=True,
                daily_seasonality=False
            )
            if span_days >= 365:
                m.add_seasonality(name='yearly', period=365, fourier_order=5)
                
            m.fit(daily_sales)
            
            future = m.make_future_dataframe(periods=horizon, include_history=False)
            forecast = m.predict(future)
            
            forecast_points = []
            forecast_total = 0.0
            
            for _, row in forecast.iterrows():
                val = max(0.0, float(row['yhat']))
                forecast_points.append({
                    "ds": row['ds'].strftime("%Y-%m-%d"),
                    "yhat": val,
                    "yhat_lower": max(0.0, float(row['yhat_lower'])),
                    "yhat_upper": float(row['yhat_upper'])
                })
                forecast_total += val
                
            # Trend slope (difference between end and start values divided by duration)
            trend_slope = 0.0
            if len(forecast) > 1:
                trend_slope = (float(forecast.iloc[-1]['yhat']) - float(forecast.iloc[0]['yhat'])) / horizon
                
            logger.debug(f"Prophet forecast completed successfully for horizon={horizon}")
            return forecast_total, trend_slope, forecast_points, "Prophet"
            
    except Exception as e:
        logger.warning(f"Prophet forecast failed, falling back to Linear Regression: {e}")
        
    # ── Fallback 1: Linear Regression ──
    try:
        if len(daily_sales) >= 4:
            # Create numeric indices for X
            daily_sales['time_index'] = np.arange(len(daily_sales))
            X = daily_sales[['time_index']]
            y = daily_sales['y']
            
            lr = LinearRegression()
            lr.fit(X, y)
            
            # Predict future indices
            future_indices = np.arange(len(daily_sales), len(daily_sales) + horizon).reshape(-1, 1)
            predictions = lr.predict(future_indices)
            
            forecast_points = []
            forecast_total = 0.0
            
            for i, pred_val in enumerate(predictions):
                # Clamp prediction at 0
                val = max(0.0, float(pred_val))
                fcst_date = start_date + timedelta(days=i+1)
                
                # Standard error approximation for CI
                std_err = float(np.std(y)) if len(y) > 1 else 1.0
                
                forecast_points.append({
                    "ds": fcst_date.strftime("%Y-%m-%d"),
                    "yhat": val,
                    "yhat_lower": max(0.0, val - 1.96 * std_err),
                    "yhat_upper": val + 1.96 * std_err
                })
                forecast_total += val
                
            trend_slope = float(lr.coef_[0])
            logger.info("Linear Regression fallback completed successfully")
            return forecast_total, trend_slope, forecast_points, "Linear Regression"
            
    except Exception as e:
        logger.warning(f"Linear Regression forecast failed, falling back to Moving Average: {e}")
        
    # ── Fallback 2: Moving Average ──
    try:
        avg_sales = 0.0
        if not daily_sales.empty:
            avg_sales = max(0.0, float(daily_sales['y'].mean()))
            
        forecast_points = []
        forecast_total = 0.0
        
        for i in range(horizon):
            fcst_date = start_date + timedelta(days=i+1)
            val = avg_sales
            
            forecast_points.append({
                "ds": fcst_date.strftime("%Y-%m-%d"),
                "yhat": val,
                "yhat_lower": val * 0.8,
                "yhat_upper": val * 1.2
            })
            forecast_total += val
            
        logger.info("Moving Average fallback completed successfully")
        return forecast_total, 0.0, forecast_points, "Moving Average"
        
    except Exception as e:
        logger.error(f"All forecasting models failed: {e}", exc_info=True)
        # Final emergency return (flat 0s)
        forecast_points = []
        for i in range(horizon):
            fcst_date = start_date + timedelta(days=i+1)
            forecast_points.append({
                "ds": fcst_date.strftime("%Y-%m-%d"),
                "yhat": 0.0,
                "yhat_lower": 0.0,
                "yhat_upper": 0.0
            })
        return 0.0, 0.0, forecast_points, "Emergency Fallback"
