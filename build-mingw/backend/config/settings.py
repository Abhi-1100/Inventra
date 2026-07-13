"""
Configuration Settings for the AI Inventory Management Backend.
Contains database paths, logging options, model persistence directory, and pipeline parameters.
"""

import os
from pathlib import Path
from typing import Dict

# Base paths
BASE_DIR = Path(__file__).resolve().parent.parent
DEFAULT_DB_PATH = BASE_DIR / "inventory.db"
MODEL_DIR = BASE_DIR / "saved_models"

# Ensure directories exist
os.makedirs(MODEL_DIR, exist_ok=True)

# Application Versioning
VERSION = "1.0.0"

# Forecasting Configuration
DEFAULT_FORECAST_HORIZON_DAYS = 7
MIN_WEEKS_FOR_ML = 4  # Cold start limit: 4 weeks * 7 = 28 days

# Economic Order Quantity Defaults
DEFAULT_ORDERING_COST = 20.0      # S: cost per order
DEFAULT_HOLDING_COST_RATE = 0.25  # H: 25% of unit cost per year
DEFAULT_LEAD_TIME_DAYS = 7        # L: lead time for delivery

# Priority Score Configuration
# Weights must sum to 1.0
PRIORITY_WEIGHTS: Dict[str, float] = {
    "forecast_demand": 0.25,
    "stock_level": 0.35,
    "lead_time": 0.15,
    "trend": 0.15,
    "avg_usage": 0.10,
}

# RandomForestClassifier Config
RF_PARAMS = {
    "n_estimators": 100,
    "max_depth": 5,
    "random_state": 42,
}

# Logger Configuration
LOG_FILE = BASE_DIR / "backend.log"
LOG_FORMAT = "%(asctime)s - %(name)s - %(levelname)s - %(message)s"
LOG_LEVEL = "INFO"
