"""
Input Validation Utilities for the AI Inventory Management Backend.
Defines exceptions and validation logic for products, entries, movements, and CSVs.
"""

from datetime import datetime
from typing import Dict, Any, List

class ValidationError(ValueError):
    """Exception raised for validation errors in input parameters."""
    pass

class DuplicateProductError(ValidationError):
    """Exception raised when a duplicate SKU or product name is detected."""
    pass

class DatabaseError(RuntimeError):
    """Wrapper exception for database execution failures."""
    pass

def validate_required_fields(data: Dict[str, Any], required: List[str]) -> None:
    """
    Validates that required keys exist and are not empty/None.
    """
    for field in required:
        if field not in data or data[field] is None:
            raise ValidationError(f"Missing required field: '{field}'")
        if isinstance(data[field], str) and not data[field].strip():
            raise ValidationError(f"Field '{field}' cannot be empty or whitespace.")

def validate_non_negative(value: Any, name: str) -> None:
    """
    Validates that a numeric value is non-negative (>= 0).
    """
    if not isinstance(value, (int, float)):
        try:
            value = float(value)
        except (ValueError, TypeError):
            raise ValidationError(f"Field '{name}' must be a numeric type.")
            
    if value < 0:
        raise ValidationError(f"Field '{name}' must be non-negative. Got: {value}")

def validate_positive(value: Any, name: str) -> None:
    """
    Validates that a numeric value is strictly positive (> 0).
    """
    if not isinstance(value, (int, float)):
        try:
            value = float(value)
        except (ValueError, TypeError):
            raise ValidationError(f"Field '{name}' must be a numeric type.")
            
    if value <= 0:
        raise ValidationError(f"Field '{name}' must be strictly positive. Got: {value}")

def validate_date_format(date_str: str, name: str) -> None:
    """
    Validates that a string matches 'YYYY-MM-DD' format.
    """
    if not isinstance(date_str, str):
        raise ValidationError(f"Field '{name}' must be a string date.")
    try:
        datetime.strptime(date_str, "%Y-%m-%d")
    except ValueError:
        raise ValidationError(f"Field '{name}' must be in YYYY-MM-DD format. Got: '{date_str}'")

def validate_product_data(sku: str, name: str, unit_cost: float, current_stock: int, reorder_point: int) -> None:
    """
    Validates product registration inputs.
    """
    if not sku or not sku.strip():
        raise ValidationError("SKU cannot be empty.")
    if not name or not name.strip():
        raise ValidationError("Product Name cannot be empty.")
        
    validate_non_negative(unit_cost, "unit_cost")
    validate_non_negative(current_stock, "current_stock")
    validate_non_negative(reorder_point, "reorder_point")

def validate_daily_entry_data(product_id: int, entry_date: str, units_sold: int, units_wasted: int) -> None:
    """
    Validates daily inventory sales and waste entries.
    """
    validate_positive(product_id, "product_id")
    validate_date_format(entry_date, "entry_date")
    validate_non_negative(units_sold, "units_sold")
    validate_non_negative(units_wasted, "units_wasted")

def validate_stock_movement_data(product_id: int, movement_type: str, quantity: int, movement_date: str) -> None:
    """
    Validates stock movement ledger entries.
    """
    validate_positive(product_id, "product_id")
    if movement_type not in ("IN", "OUT"):
        raise ValidationError(f"movement_type must be either 'IN' or 'OUT'. Got: '{movement_type}'")
    validate_positive(quantity, "quantity")
    validate_date_format(movement_date, "movement_date")
