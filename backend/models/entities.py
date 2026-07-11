"""
Data Entities for the AI Inventory Management Backend.
Defines dataclasses for database entities and ML pipeline results.
"""

from dataclasses import dataclass, field
from typing import List, Dict, Any, Optional

@dataclass
class Product:
    """
    Represents a product catalog record.
    """
    id: int = 0
    sku: str = ""
    name: str = ""
    category: str = ""
    current_stock: int = 0
    unit_cost: float = 0.0
    reorder_point: int = 10
    created_at: Optional[str] = None
    updated_at: Optional[str] = None

@dataclass
class DailyEntry:
    """
    Represents a daily inventory sale and waste record.
    """
    id: int = 0
    product_id: int = 0
    entry_date: str = ""
    units_sold: int = 0
    units_wasted: int = 0
    entered_by: Optional[int] = None
    created_at: Optional[str] = None
    product_name: Optional[str] = None
    product_sku: Optional[str] = None

@dataclass
class StockMovement:
    """
    Represents a stock movement ledger entry (IN or OUT).
    """
    id: int = 0
    product_id: int = 0
    movement_type: str = ""  # 'IN' or 'OUT'
    quantity: int = 0
    supplier_name: str = ""
    cost_per_unit: float = 0.0
    reason: str = ""
    movement_date: str = ""
    entered_by: Optional[int] = None
    created_at: Optional[str] = None
    product_name: Optional[str] = None
    staff_name: Optional[str] = None

@dataclass
class ForecastPoint:
    """
    Represents a single day's forecast.
    """
    ds: str = ""
    yhat: float = 0.0
    yhat_lower: float = 0.0
    yhat_upper: float = 0.0

@dataclass
class MLResult:
    """
    Represents the output of the ML pipeline for a single product.
    """
    product_id: int = 0
    sku: str = ""
    demand_label: str = "Medium"  # 'High', 'Medium', 'Low'
    stock_status: str = "NoAction"  # 'Reorder', 'Overstock', 'NoAction'
    confidence: float = 0.0        # percentage 0-100
    forecast_7d: float = 0.0
    trend: float = 0.0
    eoq: int = 0
    priority: str = "Safe"         # 'Critical', 'ReorderSoon', 'Safe'
    explanations: str = ""
    forecast_points: List[Dict[str, Any]] = field(default_factory=list)

@dataclass
class ShopProfile:
    """
    Represents the shop metadata.
    """
    id: int = 1
    name: str = ""
    owner_name: str = ""
    phone: str = ""
    logo_path: str = ""

@dataclass
class StaffUser:
    """
    Represents an authorized application user.
    """
    id: int = 0
    name: str = ""
    role: str = ""  # 'Owner' or 'Staff'
    pin_hash: str = ""
    created_at: Optional[str] = None
