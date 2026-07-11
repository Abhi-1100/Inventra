import pandas as pd
import numpy as np
from sklearn.cluster import KMeans
from sklearn.svm import SVC
from sklearn.model_selection import cross_val_score
from sklearn.preprocessing import StandardScaler

def run_pipeline(df: pd.DataFrame) -> dict:
    """
    K-Means clustering to assign Demand Tiers (High/Medium/Low).
    SVM classification to predict reorder recommendations.
    
    Returns a dict mapping SKU/Product identifier to features.
    """
    results = {}
    
    # ── 1. Group by SKU to calculate aggregate features ──
    agg_df = df.groupby(['sku', 'name', 'category']).agg({
        'sales_volume': ['sum', 'mean', 'std'],
        'current_stock': 'last',
        'unit_cost': 'mean'
    }).reset_index()
    
    # Flatten columns
    agg_df.columns = ['sku', 'name', 'category', 'total_sales', 'avg_daily_sales', 'sales_std', 'current_stock', 'unit_cost']
    agg_df['sales_std'] = agg_df['sales_std'].fillna(0.0)
    
    # ── 2. Demand Tier Clustering (K-Means) ──
    # High/Medium/Low clustering based on daily sales averages
    if len(agg_df) >= 3:
        kmeans = KMeans(n_clusters=3, random_state=42, n_init='auto')
        sales_features = agg_df[['avg_daily_sales']].fillna(0.0)
        agg_df['demand_cluster'] = kmeans.fit_predict(sales_features)
        
        # Order clusters by mean daily sales
        cluster_means = agg_df.groupby('demand_cluster')['avg_daily_sales'].mean().sort_values()
        rank_map = {cluster_means.index[0]: "Low", cluster_means.index[1]: "Medium", cluster_means.index[2]: "High"}
        agg_df['demand_label'] = agg_df['demand_cluster'].map(rank_map)
    else:
        # Fallback if too few products
        agg_df['demand_label'] = "Medium"
        
    # ── 3. SVM Reorder Action Classification ──
    # Create simple ground truth heuristics to train SVM classifier model
    # (High risk of stockout = Reorder, High stock with low sales = Overstock, else No Action)
    def determine_heuristic_label(row):
        # Heuristic rules
        safety_stock = row['avg_daily_sales'] * 7  # 7 days lead safety
        if row['current_stock'] <= safety_stock:
            return "Reorder"
        elif row['current_stock'] > (row['avg_daily_sales'] * 30):
            return "Overstock"
        return "No Action"
        
    agg_df['heuristic_target'] = agg_df.apply(determine_heuristic_label, axis=1)
    
    # Extract features for SVM
    X = agg_df[['total_sales', 'current_stock', 'unit_cost']].fillna(0.0)
    y = agg_df['heuristic_target']
    
    # Train-test or simple fit (using SMOTE if available, fallback if not)
    # SMOTE is used to balance small/unbalanced datasets typical for small shops
    try:
        from imblearn.over_sampling import SMOTE
        smote = SMOTE(random_state=42, k_neighbors=min(2, len(X) - 1))
        X_res, y_res = smote.fit_resample(X, y)
    except Exception:
        # Fallback to standard data if imbalanced-learn is not installed
        X_res, y_res = X, y
        
    # Standardize features
    scaler = StandardScaler()
    X_scaled = scaler.fit_transform(X_res)
    
    # Fit Linear Support Vector Machine
    clf = SVC(kernel='linear', probability=True, random_state=42)
    clf.fit(X_scaled, y_res)
    
    # Predict probabilities for original dataset
    X_orig_scaled = scaler.transform(X)
    probs = clf.predict_proba(X_orig_scaled)
    predictions = clf.predict(X_orig_scaled)
    
    # Build final result dictionary mapped by SKU
    for idx, row in agg_df.iterrows():
        sku = row['sku']
        pred_label = predictions[idx]
        confidence = float(np.max(probs[idx])) * 100.0
        
        # Map back to structured model fields
        results[sku] = {
            "demand_label": row['demand_label'],
            "stock_status": pred_label,
            "confidence": confidence,
            "avg_sales": float(row['avg_daily_sales']),
            "current_stock": int(row['current_stock']),
            "unit_cost": float(row['unit_cost'])
        }
        
    return results
