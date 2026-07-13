"""
Helper Functions for the AI Inventory Management Backend.
Provides date formatting and data hashing utilities.
"""

import hashlib
import pandas as pd
from datetime import datetime, timedelta
from typing import Union

def compute_data_hash(df: pd.DataFrame) -> str:
    """
    Computes a SHA-256 hash of a pandas DataFrame to detect changes.
    Used for model retraining optimization.
    """
    if df.empty:
        return hashlib.sha256(b"").hexdigest()
    
    # Sort by index and columns to ensure deterministic hash
    sorted_df = df.sort_index().reindex(sorted(df.columns), axis=1)
    
    # Convert to string and hash
    df_str = sorted_df.to_string().encode("utf-8")
    return hashlib.sha256(df_str).hexdigest()

def get_weeks_between(start_date: Union[str, datetime], end_date: Union[str, datetime]) -> float:
    """
    Calculates the number of weeks between two dates.
    """
    if isinstance(start_date, str):
        start_date = datetime.strptime(start_date, "%Y-%m-%d")
    if isinstance(end_date, str):
        end_date = datetime.strptime(end_date, "%Y-%m-%d")
        
    delta = abs(end_date - start_date)
    return delta.days / 7.0

def get_season(month: int) -> int:
    """
    Returns a numeric value representing the season for a given month.
    1: Spring (March - May)
    2: Summer (June - August)
    3: Autumn (September - November)
    4: Winter (December - February)
    """
    if month in (3, 4, 5):
        return 1
    elif month in (6, 7, 8):
        return 2
    elif month in (9, 10, 11):
        return 3
    else:
        return 4
