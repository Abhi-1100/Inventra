"""
Economic Order Quantity (EOQ) calculator for the AI Inventory Management Backend.
Implements the Wilson EOQ formula and handles division by zero or negative costs.
"""

import math
from backend.utils.logger import get_logger

logger = get_logger("eoq")

def calculate_eoq(
    demand: float,
    ordering_cost: float,
    holding_cost_rate: float,
    unit_cost: float
) -> int:
    """
    Standard Wilson EOQ Formula:
    EOQ = sqrt( (2 * Annual Demand * Ordering Cost) / (Holding Cost Rate * Unit Cost) )
    
    Args:
        demand: Average demand per day.
        ordering_cost: Constant setup/ordering cost (S).
        holding_cost_rate: Annual holding cost rate (fraction of unit cost, e.g. 0.25).
        unit_cost: Cost per unit (C).
        
    Returns:
        int: Optimal Economic Order Quantity. Returns 0 on invalid/zero costs.
    """
    if demand <= 0 or ordering_cost <= 0 or holding_cost_rate <= 0 or unit_cost <= 0:
        return 0
        
    try:
        # Convert daily demand to annual demand
        annual_demand = demand * 365.0
        
        # Holding cost per unit per year
        holding_cost = holding_cost_rate * unit_cost
        
        if holding_cost <= 0:
            return 0
            
        eoq_value = math.sqrt((2.0 * annual_demand * ordering_cost) / holding_cost)
        
        # Round to nearest integer and clamp at minimum of 1
        return int(max(1, round(eoq_value)))
        
    except Exception as e:
        logger.error(f"Error calculating EOQ (demand={demand}, cost={unit_cost}): {e}")
        return 0
