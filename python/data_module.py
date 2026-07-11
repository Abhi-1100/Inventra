"""
C++ Python Bridge Wrapper for data loading.
Delegates to backend verification and cleaning utilities.
"""

import pandas as pd
from backend.services.inventory_service import InventoryService
from backend.database.db_manager import DBManager

def load_csv(file_path: str) -> pd.DataFrame:
    """
    Loads raw inventory history CSV and cleans it using pandas.
    Matches expectations of C++ caller.
    """
    df = pd.read_csv(file_path)
    
    # Standardize column headers (strip, lowercase, replace spaces with underscores)
    df.columns = [c.strip().lower().replace(" ", "_") for c in df.columns]
    
    # Fill missing values and enforce correct types
    df['current_stock'] = pd.to_numeric(df.get('current_stock', 0), errors='coerce').fillna(0).astype(int)
    df['unit_cost'] = pd.to_numeric(df.get('unit_cost', 0.0), errors='coerce').fillna(0.0)
    df['sales_volume'] = pd.to_numeric(df.get('sales_volume', 0), errors='coerce').fillna(0).astype(int)
    
    if 'date' in df.columns:
        df['date'] = pd.to_datetime(df['date'], errors='coerce')
        
    return df

def validate_columns(df: pd.DataFrame) -> list:
    """
    Validates if mandatory columns exist.
    """
    required = ['sku', 'name', 'current_stock', 'unit_cost']
    missing = [col for col in required if col not in df.columns]
    return missing
