import pandas as pd
import numpy as np

def load_csv(file_path: str) -> pd.DataFrame:
    """
    Loads raw grocery inventory history CSV and cleans it.
    Expects columns resembling:
    - sku
    - name
    - category
    - current_stock
    - unit_cost
    - date (sales history date)
    - sales_volume
    """
    df = pd.read_csv(file_path)
    
    # Clean column headers (strip whitespaces, lowercase, replace spaces)
    df.columns = [c.strip().lower().replace(" ", "_") for c in df.columns]
    
    # Fill standard missing values
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
