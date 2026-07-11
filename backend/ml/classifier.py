"""
Classification Engine for the AI Inventory Management Backend.
Trains a RandomForestClassifier to predict inventory health status:
- Critical
- Low
- Healthy
- Overstock
Retrieves confidence thresholds directly via predict_proba.
"""

import numpy as np
import pandas as pd
from typing import List, Dict, Any, Tuple
from sklearn.ensemble import RandomForestClassifier
from sklearn.preprocessing import StandardScaler

from backend.config import settings
from backend.utils.logger import get_logger

logger = get_logger("classifier")

# Status Class mapping
CLASSES = ["Critical", "Low", "Healthy", "Overstock"]

def determine_heuristic_target(row: Dict[str, Any]) -> str:
    """
    Applies logic to assign heuristic labels for bootstrapping training data.
    
    Heuristic:
      - Critical: Current stock <= average usage * 2 days
      - Low: Current stock <= reorder point
      - Overstock: Current stock > average usage * 30 days
      - Healthy: Otherwise
    """
    current_stock = float(row.get("current_stock", 0))
    avg_usage = float(row.get("avg_usage", 5.0))
    reorder_point = float(row.get("reorder_point", 10))
    
    safety_stock = avg_usage * 2.0
    overstock_threshold = max(reorder_point * 3.0, avg_usage * 30.0)
    
    if current_stock <= safety_stock:
        return "Critical"
    elif current_stock <= reorder_point:
        return "Low"
    elif current_stock > overstock_threshold:
        return "Overstock"
    return "Healthy"

def train_classifier(
    features_df: pd.DataFrame,
    feature_cols: List[str]
) -> Tuple[RandomForestClassifier, StandardScaler, float]:
    """
    Trains a RandomForestClassifier on inventory features.
    Bootstraps targets using heuristic rules if target is not already present.
    
    Returns:
        Tuple containing:
            - RandomForestClassifier: The fitted model.
            - StandardScaler: The fitted scaler.
            - accuracy (float): Training accuracy score.
    """
    df = features_df.copy()
    
    # 1. Bootstrap Target Column if missing
    if "target" not in df.columns:
        # Generate target per row
        df["target"] = df.apply(determine_heuristic_target, axis=1)
        
    X = df[feature_cols].fillna(0.0)
    y = df["target"]
    
    # Check if we have enough samples
    if len(df) == 0:
        raise ValueError("Cannot train classifier on empty dataset.")
        
    # Scale features
    scaler = StandardScaler()
    X_scaled = scaler.fit_transform(X)
    
    # Train Random Forest
    clf = RandomForestClassifier(**settings.RF_PARAMS)
    clf.fit(X_scaled, y)
    
    # Compute simple training accuracy
    predictions = clf.predict(X_scaled)
    acc = float(np.mean(predictions == y))
    
    logger.info(f"Classifier trained successfully. Train Accuracy: {acc * 100.0:.2f}%")
    return clf, scaler, acc

def predict_health(
    clf: RandomForestClassifier,
    scaler: StandardScaler,
    feature_row: Dict[str, Any],
    feature_cols: List[str]
) -> Tuple[str, float]:
    """
    Predicts inventory status and confidence.
    
    Returns:
        Tuple[str, float]: Predicted label and confidence score (0.0 to 100.0).
    """
    try:
        # Convert row dict to dataframe matching feature columns
        row_df = pd.DataFrame([feature_row])[feature_cols].fillna(0.0)
        
        # Scale
        X_scaled = scaler.transform(row_df)
        
        # Predict Class and Probability
        pred_label = clf.predict(X_scaled)[0]
        probs = clf.predict_proba(X_scaled)[0]
        
        # Confidence is the max probability
        class_idx = list(clf.classes_).index(pred_label)
        confidence = float(probs[class_idx]) * 100.0
        
        return str(pred_label), confidence
        
    except Exception as e:
        logger.error(f"Error during status classification prediction: {e}", exc_info=True)
        # Fallback to heuristic
        fallback_label = determine_heuristic_target(feature_row)
        return fallback_label, 50.0
