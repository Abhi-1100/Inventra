"""
Explainable AI (XAI) Engine for the AI Inventory Management Backend.
Generates clear, plain-English justifications for reorder points, stock statuses, and priorities.
Provides direct business utility for inventory managers.
"""

from backend.utils.logger import get_logger

logger = get_logger("explainability")

def generate_explanation(
    sku: str,
    name: str,
    current_stock: int,
    reorder_point: int,
    forecast_7d: float,
    lead_time: int,
    trend: float,
    eoq: int,
    priority: str,
    stock_status: str
) -> str:
    """
    Constructs a plain-English explanation for an item's prediction.
    
    Args:
        sku: Product SKU.
        name: Product name.
        current_stock: Current units on hand.
        reorder_point: Product reorder threshold.
        forecast_7d: 7-day demand forecast.
        lead_time: Lead time in days.
        trend: Trend slope (units/day).
        eoq: Computed Economic Order Quantity.
        priority: Priority rank ('Critical', 'ReorderSoon', 'Safe').
        stock_status: Predicted health status ('Critical', 'Low', 'Healthy', 'Overstock').
        
    Returns:
        str: Business explanation sentence.
    """
    try:
        explanations = []
        
        # 1. Analyze Stock Level relative to reorder point
        if current_stock == 0:
            explanations.append(f"Product '{name}' (SKU: {sku}) is completely out of stock.")
        elif stock_status == "Critical" or current_stock <= reorder_point * 0.5:
            explanations.append(f"For product '{name}' (SKU: {sku}), current stock of {current_stock} units is critically low, sitting far below the reorder point of {reorder_point}.")
        elif current_stock <= reorder_point:
            explanations.append(f"For product '{name}' (SKU: {sku}), current stock of {current_stock} units has fallen below the reorder point of {reorder_point}.")
        elif stock_status == "Overstock":
            explanations.append(f"For product '{name}' (SKU: {sku}), current stock of {current_stock} units is excessively high relative to demand.")
        else:
            explanations.append(f"For product '{name}' (SKU: {sku}), current stock of {current_stock} units is at a healthy level (above the reorder point of {reorder_point}).")
            
        # 2. Demand Forecast and Trend
        rounded_forecast = round(forecast_7d, 1)
        trend_direction = "rising" if trend > 0.05 else "falling" if trend < -0.05 else "stable"
        
        if rounded_forecast > 0:
            explanations.append(f"Forecast demand for next week is {rounded_forecast} units, with a {trend_direction} trend.")
        else:
            explanations.append("Forecast demand for next week is close to zero, with a stable trend.")
            
        # 3. Action recommendations based on Lead time and priority
        if stock_status in ("Critical", "Low") or priority in ("Critical", "ReorderSoon"):
            risk = "immediate stockout" if stock_status == "Critical" else "potential stockout"
            explanations.append(
                f"Given the {lead_time}-day lead time, ordering the Economic Order Quantity (EOQ) of {eoq} units now "
                f"is recommended to reduce the risk of {risk} (Priority: {priority})."
            )
        elif stock_status == "Overstock":
            explanations.append("It is advised to halt ordering and allow stock levels to deplete through sales.")
        else:
            explanations.append(f"No action is required at this time. Re-evaluation will occur next week.")
            
        return " ".join(explanations)
        
    except Exception as e:
        logger.error(f"Failed to generate explanation for SKU {sku}: {e}")
        return f"Prediction for product '{name}' suggests a status of '{stock_status}' and priority '{priority}'."
