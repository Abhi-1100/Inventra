"""
Unit Tests for Utilities (Validators & Helpers).
Tests input validations, range checks, type validations, and datetime helpers.
"""

import pytest
from datetime import datetime
from backend.utils.helpers import get_weeks_between, get_season, compute_data_hash
from backend.utils.validators import (
    ValidationError,
    validate_required_fields,
    validate_non_negative,
    validate_positive,
    validate_date_format,
    validate_product_data,
    validate_daily_entry_data,
    validate_stock_movement_data
)

def test_helpers():
    """Tests date and hashing utility functions."""
    # Season checks
    assert get_season(3) == 1
    assert get_season(4) == 1
    assert get_season(5) == 1
    assert get_season(6) == 2
    assert get_season(9) == 3
    assert get_season(12) == 4
    
    # Weeks between
    d1 = datetime(2026, 7, 1)
    d2 = datetime(2026, 7, 8)
    assert get_weeks_between(d1, d2) == 1.0
    
    # Empty DataFrame hash
    import pandas as pd
    assert len(compute_data_hash(pd.DataFrame())) == 64

def test_validators_required_fields():
    """Tests key existence and non-emptiness validations."""
    # Happy path
    validate_required_fields({"a": "ok", "b": 1}, ["a", "b"])
    
    # Missing key
    with pytest.raises(ValidationError, match="Missing required field: 'b'"):
        validate_required_fields({"a": "ok"}, ["a", "b"])
        
    # Empty string
    with pytest.raises(ValidationError, match="Field 'a' cannot be empty or whitespace"):
        validate_required_fields({"a": "   ", "b": 1}, ["a", "b"])

def test_validators_non_negative():
    """Tests non-negative constraints."""
    # Happy path
    validate_non_negative(0, "val")
    validate_non_negative(5.5, "val")
    validate_non_negative("10", "val")
    
    # Negative
    with pytest.raises(ValidationError, match="Field 'val' must be non-negative"):
        validate_non_negative(-1, "val")
        
    # Non-numeric
    with pytest.raises(ValidationError, match="Field 'val' must be a numeric type"):
        validate_non_negative("abc", "val")

def test_validators_positive():
    """Tests strictly positive constraints."""
    # Happy path
    validate_positive(1, "val")
    validate_positive(0.1, "val")
    
    # Zero or negative
    with pytest.raises(ValidationError, match="Field 'val' must be strictly positive"):
        validate_positive(0, "val")
    with pytest.raises(ValidationError, match="Field 'val' must be strictly positive"):
        validate_positive(-5, "val")

def test_validators_date_format():
    """Tests date string YYYY-MM-DD parsing validations."""
    validate_date_format("2026-07-11", "date")
    
    with pytest.raises(ValidationError, match="must be in YYYY-MM-DD format"):
        validate_date_format("11-07-2026", "date")
    with pytest.raises(ValidationError, match="must be a string date"):
        validate_date_format(12345, "date")

def test_validators_composite_records():
    """Tests validation logic for products, entries, and movements."""
    # Product data invalid SKU or Name
    with pytest.raises(ValidationError, match="SKU cannot be empty"):
        validate_product_data("   ", "Name", 1.0, 5, 5)
    with pytest.raises(ValidationError, match="Product Name cannot be empty"):
        validate_product_data("SKU", "", 1.0, 5, 5)
        
    # Daily entry data
    validate_daily_entry_data(1, "2026-07-11", 5, 0)
    with pytest.raises(ValidationError):
        validate_daily_entry_data(-1, "2026-07-11", 5, 0)
        
    # Stock movement data
    validate_stock_movement_data(1, "IN", 10, "2026-07-11")
    with pytest.raises(ValidationError, match="movement_type must be either"):
        validate_stock_movement_data(1, "BAD", 10, "2026-07-11")
