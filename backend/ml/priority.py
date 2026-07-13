"""
Priority Scoring System for the AI Inventory Management Backend.
Computes a ranking priority score (0-100) using multi-criteria normalization.
Supports custom weights for flexibility.
"""

import math
from typing import Dict, Optional
from backend.config import settings
from backend.utils.logger import get_logger

logger = get_logger("priority")

def calculate_priority_score(
    forecast_7d: float,
    current_stock: int,
    reorder_point: int,
    lead_time: int,
    trend: float,
    average_usage: float,
    stock_status: str,
    weights: Optional[Dict[str, float]] = None
) -> float:
    """
    Computes a priority score from 0 to 100, where higher means more urgent reorder.
    
    Args:
        forecast_7d: Predicted sales for the next 7 days.
        current_stock: Current units on hand.
        reorder_point: Reorder point of the product.
        lead_time: Supplier lead time in days.
        trend: Demand trend slope (units/day).
        average_usage: Daily average usage.
        stock_status: Predicted health category ('Critical', 'Low', 'Healthy', 'Overstock').
        weights: Optional dictionary of weights for each factor.
        
    Returns:
        float: Priority score bounded [0.0, 100.0].
    """
    if weights is None:
        weights = settings.PRIORITY_WEIGHTS
        
    try:
        # ── 1. Normalize Stock Level (Urgency increases as stock drops relative to reorder point) ──
        # If stock is 0 -> 1.0 priority. If stock is >= 2 * reorder_point -> 0.0 priority.
        denom = float(max(1, reorder_point * 2))
        stock_factor = max(0.0, min(1.0, 1.0 - (float(current_stock) / denom)))
        
        # ── 2. Normalize Forecast Demand (High upcoming demand increases urgency) ──
        # Normalize relative to expected weekly usage. If forecast is double the usage -> 1.0.
        expected_weekly = float(max(1.0, average_usage * 7.0))
        forecast_factor = max(0.0, min(1.0, float(forecast_7d) / (expected_weekly * 2.0)))
        
        # ── 3. Normalize Lead Time (Longer lead times increase reorder urgency) ──
        # Normalize relative to a max of 30 days
        lead_time_factor = max(0.0, min(1.0, float(lead_time) / 30.0))
        
        # ── 4. Normalize Demand Trend (Rising trend increases urgency) ──
        # Sigmoid function maps trend slope [-inf, +inf] to [0, 1]
        try:
            trend_factor = 1.0 / (1.0 + math.exp(-float(trend)))
        except OverflowError:
            trend_factor = 1.0 if trend > 0 else 0.0
            
        # ── 5. Normalize Average Usage (Higher average velocity items are slightly prioritized) ──
        # Normalize relative to 100 units/day
        usage_factor = max(0.0, min(1.0, float(average_usage) / 100.0))
        
        # ── 6. Incorporate Stock Status Category ──
        status_map = {
            "Critical": 1.0,
            "Low": 0.7,
            "Reorder": 0.7,
            "Healthy": 0.2,
            "No Action": 0.2,
            "Overstock": 0.0
        }
        status_factor = status_map.get(stock_status, 0.2)
        
        # Weighted aggregate
        score = (
            weights.get("stock_level", 0.35) * stock_factor +
            weights.get("forecast_demand", 0.25) * forecast_factor +
            weights.get("lead_time", 0.15) * lead_time_factor +
            weights.get("trend", 0.15) * trend_factor +
            weights.get("avg_usage", 0.10) * usage_factor
        )
        
        # Scale to 0-100 range and blend with status factor
        scaled_score = float(score * 100.0)
        
        # Adjust score slightly using status factor to ensure Critical items are always highest
        final_score = float(scaled_score * 0.7 + status_factor * 30.0)
        
        return max(0.0, min(100.0, round(final_score, 2)))
        
    except Exception as e:
        logger.error(f"Error calculating priority score: {e}")
        return 50.0  # Safe neutral score
