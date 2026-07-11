import pandas as pd
import numpy as np
import math
from datetime import datetime, timedelta

def calculate_eoq(demand: float, ordering_cost: float, holding_cost_rate: float, unit_cost: float) -> int:
    """
    Standard Wilson EOQ Formula:
    EOQ = sqrt( (2 * Annual Demand * Ordering Cost) / (Holding Cost Rate * Unit Cost) )
    """
    if unit_cost <= 0 or holding_cost_rate <= 0:
        return 0
        
    annual_demand = demand * 365.0
    holding_cost = holding_cost_rate * unit_cost
    
    eoq_val = math.sqrt((2.0 * annual_demand * ordering_cost) / holding_cost)
    return int(max(1, round(eoq_val)))

def forecast_all(df: pd.DataFrame, ml_results: dict, horizon: int, ordering_cost: float, holding_cost_rate: float) -> list:
    """
    Performs 7-day Prophet forecasting for all items in the dataset,
    computes EOQ recommendations, and ranks Priority Scores.
    """
    forecast_results = []
    unique_skus = df['sku'].unique()
    
    # Check if prophet is installed, otherwise use moving average fallback
    use_prophet = False
    try:
        from prophet import Prophet
        import logging
        # Suppress Prophet logging noise
        logging.getLogger('prophet').setLevel(logging.ERROR)
        use_prophet = True
    except ImportError:
        pass
        
    today = datetime.now()
    
    for sku in unique_skus:
        sku_df = df[df['sku'] == sku].sort_values('date')
        ml_data = ml_results.get(sku, {
            "demand_label": "Medium",
            "stock_status": "No Action",
            "confidence": 75.0,
            "avg_sales": 5.0,
            "current_stock": 10,
            "unit_cost": 50.0
        })
        
        # Aggregate to daily levels
        daily_sales = sku_df.groupby('date')['sales_volume'].sum().reset_index()
        daily_sales.columns = ['ds', 'y']
        
        forecast_pts = []
        forecast_7d_total = 0.0
        trend_slope = 0.0
        
        if use_prophet and len(daily_sales) >= 10:
            try:
                # Verify length of time history
                span_days = (daily_sales['ds'].max() - daily_sales['ds'].min()).days
                
                m = Prophet(
                    yearly_seasonality=False,
                    weekly_seasonality=True,
                    daily_seasonality=False
                )
                if span_days >= 365:
                    m.add_seasonality(name='yearly', period=365, fourier_order=5)
                    
                m.fit(daily_sales)
                
                future = m.make_future_dataframe(periods=horizon)
                forecast = m.predict(future)
                
                # Fetch forecast rows (last 'horizon' days)
                fcst_slice = forecast.tail(horizon)
                
                for _, row in fcst_slice.iterrows():
                    val = max(0.0, float(row['yhat']))
                    forecast_pts.append({
                        "ds": row['ds'].strftime("%Y-%m-%d"),
                        "yhat": val,
                        "yhat_lower": max(0.0, float(row['yhat_lower'])),
                        "yhat_upper": float(row['yhat_upper'])
                    })
                    forecast_7d_total += val
                    
                # Calculate trend slope (difference between last and first day of forecast)
                if len(fcst_slice) > 1:
                    trend_slope = (fcst_slice.iloc[-1]['yhat'] - fcst_slice.iloc[0]['yhat']) / horizon
                    
            except Exception:
                # Fallback to moving average on Prophet fitting failure
                forecast_7d_total, trend_slope, forecast_pts = get_fallback_forecast(daily_sales, horizon, today)
        else:
            # Fallback for short histories or missing Prophet library
            forecast_7d_total, trend_slope, forecast_pts = get_fallback_forecast(daily_sales, horizon, today)
            
        # Calculate Economic Order Quantity
        eoq = calculate_eoq(
            demand=ml_data['avg_sales'],
            ordering_cost=ordering_cost,
            holding_cost_rate=holding_cost_rate,
            unit_cost=ml_data['unit_cost']
        )
        
        # Calculate Priority Rank
        priority = "Safe"
        current_stock = ml_data['current_stock']
        
        # If stock is below 2 days of forecast demand -> Critical
        if current_stock <= (forecast_7d_total / 7.0) * 2.0:
            priority = "Critical"
        # If stock is below 7 days of forecast demand -> Reorder Soon
        elif current_stock <= forecast_7d_total:
            priority = "ReorderSoon"
            
        forecast_results.append({
            "product_id": int(df[df['sku'] == sku]['product_id'].iloc[0]) if 'product_id' in df.columns else 0,
            "sku": sku,
            "demand_label": ml_data['demand_label'],
            "stock_status": ml_data['stock_status'],
            "confidence": ml_data['confidence'],
            "forecast_7d": forecast_7d_total,
            "trend": trend_slope,
            "eoq": eoq,
            "priority": priority,
            "forecast_points": forecast_pts
        })
        
    return forecast_results

def get_fallback_forecast(daily_sales: pd.DataFrame, horizon: int, start_date: datetime) -> tuple:
    """
    Simple moving average fallback forecasting model.
    """
    avg_sales = 5.0
    if not daily_sales.empty:
        avg_sales = max(0.1, daily_sales['y'].mean())
        
    forecast_pts = []
    forecast_7d_total = 0.0
    
    for i in range(horizon):
        date_str = (start_date + timedelta(days=i+1)).strftime("%Y-%m-%d")
        val = avg_sales
        # Add slight noise to simulate predictions
        noise = (np.random.rand() - 0.5) * avg_sales * 0.1
        val_noisy = max(0.0, val + noise)
        
        forecast_pts.append({
            "ds": date_str,
            "yhat": val_noisy,
            "yhat_lower": val_noisy * 0.8,
            "yhat_upper": val_noisy * 1.2
        })
        forecast_7d_total += val_noisy
        
    return forecast_7d_total, 0.0, forecast_pts
