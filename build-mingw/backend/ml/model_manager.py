"""
Model Manager for the AI Inventory Management Backend.
Handles saving and loading of trained models (Random Forest, scalers) using Joblib.
Manages metadata (version, hash, features) and avoids redundant retraining.
"""

import os
import json
import joblib
from datetime import datetime
from typing import Dict, Any, List, Tuple, Optional
from backend.config import settings
from backend.utils.logger import get_logger

logger = get_logger("model_manager")

METADATA_FILE = os.path.join(settings.MODEL_DIR, "model_metadata.json")

def save_model(
    clf: Any,
    scaler: Any,
    feature_cols: List[str],
    data_hash: str
) -> None:
    """
    Saves the Random Forest classifier, scaler, and metadata to disk.
    """
    try:
        os.makedirs(settings.MODEL_DIR, exist_ok=True)
        
        clf_path = os.path.join(settings.MODEL_DIR, "classifier.joblib")
        scaler_path = os.path.join(settings.MODEL_DIR, "scaler.joblib")
        
        # Save models
        joblib.dump(clf, clf_path)
        joblib.dump(scaler, scaler_path)
        
        # Build metadata
        metadata = {
            "version": settings.VERSION,
            "training_timestamp": datetime.now().isoformat(),
            "feature_list": feature_cols,
            "data_hash": data_hash,
            "model_type": type(clf).__name__
        }
        
        with open(METADATA_FILE, "w", encoding="utf-8") as f:
            json.dump(metadata, f, indent=4)
            
        logger.info(f"Classifier and Scaler successfully saved to {settings.MODEL_DIR}")
        
    except Exception as e:
        logger.error(f"Failed to save models: {e}", exc_info=True)
        raise RuntimeError(f"Model persistence failed: {e}") from e

def load_model() -> Tuple[Optional[Any], Optional[Any], Optional[Dict[str, Any]]]:
    """
    Loads classifier, scaler, and metadata from disk.
    
    Returns:
        Tuple: (classifier, scaler, metadata). All elements are None if files do not exist.
    """
    clf_path = os.path.join(settings.MODEL_DIR, "classifier.joblib")
    scaler_path = os.path.join(settings.MODEL_DIR, "scaler.joblib")
    
    if not (os.path.exists(clf_path) and os.path.exists(scaler_path) and os.path.exists(METADATA_FILE)):
        logger.debug("No pre-trained model files found on disk.")
        return None, None, None
        
    try:
        clf = joblib.load(clf_path)
        scaler = joblib.load(scaler_path)
        
        with open(METADATA_FILE, "r", encoding="utf-8") as f:
            metadata = json.load(f)
            
        logger.info("Loaded pre-trained model and metadata from disk.")
        return clf, scaler, metadata
        
    except Exception as e:
        logger.error(f"Error loading models from disk: {e}", exc_info=True)
        return None, None, None

def should_retrain(new_data_hash: str) -> bool:
    """
    Determines if retraining is necessary by checking if the data hash has changed.
    
    Args:
        new_data_hash: Hash computed from the current training dataset.
        
    Returns:
        bool: True if retraining is required, False if cached model can be reused.
    """
    clf, scaler, metadata = load_model()
    if not clf or not scaler or not metadata:
        logger.info("Retraining required: No existing models found.")
        return True
        
    # Check if model class type is SVC
    saved_type = metadata.get("model_type")
    if saved_type != "SVC":
        logger.info(f"Retraining required: Model type changed or older model type detected (saved={saved_type}).")
        return True
        
    saved_hash = metadata.get("data_hash")
    if saved_hash != new_data_hash:
        logger.info(f"Retraining required: Data hash changed (saved={saved_hash}, new={new_data_hash}).")
        return True
        
    logger.info("Skipping retraining: Data hash is unchanged, loaded cached model.")
    return False
