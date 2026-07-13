"""
Centralized Logging Utility for the AI Inventory Management Backend.
Ensures uniform logging output to console and file.
"""

import logging
import os
from backend.config import settings

# Setup logging
logger = logging.getLogger("inventory_backend")
logger.setLevel(settings.LOG_LEVEL)

# Clear existing handlers to prevent duplicate logging
if logger.hasHandlers():
    logger.handlers.clear()

# File handler
try:
    file_handler = logging.FileHandler(settings.LOG_FILE, encoding="utf-8")
    file_handler.setLevel(settings.LOG_LEVEL)
    file_formatter = logging.Formatter(settings.LOG_FORMAT)
    file_handler.setFormatter(file_formatter)
    logger.addHandler(file_handler)
except Exception as e:
    # Fallback if log file cannot be written
    print(f"Warning: Could not configure file logger: {e}")

# Console handler
console_handler = logging.StreamHandler()
console_handler.setLevel(settings.LOG_LEVEL)
console_formatter = logging.Formatter(settings.LOG_FORMAT)
console_handler.setFormatter(console_formatter)
logger.addHandler(console_handler)

def get_logger(module_name: str) -> logging.Logger:
    """
    Returns a child logger for the calling module.
    
    Args:
        module_name: Name of the module requesting the logger.
        
    Returns:
        logging.Logger: A configured child logger.
    """
    return logger.getChild(module_name)
