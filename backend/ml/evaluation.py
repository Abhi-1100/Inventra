"""
Model Evaluation Engine for the AI Inventory Management Backend.
Calculates statistical metrics for demand forecasting and inventory classification.
Enables monitoring and logging of ML pipeline performance.
"""

import numpy as np
from typing import Dict, Any, List
from sklearn.metrics import accuracy_score, precision_score, recall_score, f1_score, mean_absolute_percentage_error, mean_squared_error, r2_score

from backend.utils.logger import get_logger

logger = get_logger("evaluation")

def evaluate_forecast(
    y_true: np.ndarray,
    y_pred: np.ndarray
) -> Dict[str, float]:
    """
    Computes performance metrics for demand forecasting.
    
    Args:
        y_true: True historical demand.
        y_pred: Predicted demand.
        
    Returns:
        Dict: MAPE, RMSE, and R2 scores.
    """
    if len(y_true) == 0 or len(y_pred) == 0:
        return {"mape": 0.0, "rmse": 0.0, "r2": 0.0}
        
    metrics = {}
    try:
        # Clamping inputs to handle edge cases
        y_true = np.array(y_true, dtype=float)
        y_pred = np.array(y_pred, dtype=float)
        
        # Calculate MAPE (safely handle zeros in y_true)
        # sklearn's MAPE handles this by using a small epsilon or default
        try:
            # Avoid division by zero by replacing zero true values with a small value
            y_true_safe = np.where(y_true == 0, 1e-5, y_true)
            metrics["mape"] = float(mean_absolute_percentage_error(y_true_safe, y_pred))
        except Exception:
            metrics["mape"] = 0.0
            
        metrics["rmse"] = float(np.sqrt(mean_squared_error(y_true, y_pred)))
        metrics["r2"] = float(r2_score(y_true, y_pred)) if len(y_true) > 1 else 1.0
        
        logger.info(f"Forecast evaluation: MAPE={metrics['mape']:.4f}, RMSE={metrics['rmse']:.4f}, R2={metrics['r2']:.4f}")
        
    except Exception as e:
        logger.error(f"Error evaluating forecasting metrics: {e}")
        metrics = {"mape": 0.0, "rmse": 0.0, "r2": 0.0}
        
    return metrics

def evaluate_classifier(
    y_true: List[str],
    y_pred: List[str]
) -> Dict[str, float]:
    """
    Computes classification report metrics for inventory status classification.
    
    Args:
        y_true: True status labels.
        y_pred: Predicted status labels.
        
    Returns:
        Dict: Accuracy, Precision, Recall, F1 scores.
    """
    if len(y_true) == 0 or len(y_pred) == 0:
        return {"accuracy": 0.0, "precision": 0.0, "recall": 0.0, "f1": 0.0}
        
    metrics = {}
    try:
        metrics["accuracy"] = float(accuracy_score(y_true, y_pred))
        metrics["precision"] = float(precision_score(y_true, y_pred, average="weighted", zero_division=0))
        metrics["recall"] = float(recall_score(y_true, y_pred, average="weighted", zero_division=0))
        metrics["f1"] = float(f1_score(y_true, y_pred, average="weighted", zero_division=0))
        
        logger.info(f"Classification evaluation: Accuracy={metrics['accuracy']:.4f}, F1={metrics['f1']:.4f}")
        
    except Exception as e:
        logger.error(f"Error evaluating classification metrics: {e}")
        metrics = {"accuracy": 0.0, "precision": 0.0, "recall": 0.0, "f1": 0.0}
        
    return metrics
