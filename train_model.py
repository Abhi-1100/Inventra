import sys
import os
import pandas as pd

# Add the current directory to sys.path so we can import backend and python modules
sys.path.insert(0, os.path.abspath(os.path.dirname(__file__)))

from python.data_module import load_csv
from backend.database.db_manager import DBManager
from backend.services.prediction_service import PredictionService

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 train_model.py <path_to_csv>")
        sys.exit(1)
        
    csv_path = sys.argv[1]
    if not os.path.exists(csv_path):
        print(f"Error: File '{csv_path}' does not exist.")
        sys.exit(1)
        
    print(f"Loading and processing dataset: {csv_path}...")
    df = load_csv(csv_path)
    print(f"Loaded {len(df)} rows. Unique products: {df['sku'].nunique()}")
    
    print("Initializing Database Manager...")
    db_manager = DBManager(":memory:")
    
    print("Initializing Prediction Service...")
    service = PredictionService(db_manager)
    
    print("Running ML Pipeline (feature extraction, training, forecasting, and EOQ)...")
    results = service.run_pipeline_with_dataframe(
        sales_df=df,
        lead_time_days=7,
        ordering_cost=20.0,
        holding_cost_rate=0.25,
        forecast_horizon=7
    )
    
    print("\n" + "="*50)
    print("ML Pipeline execution completed successfully!")
    print(f"Processed predictions for {len(results)} products.")
    print("Saved trained models in 'backend/saved_models/' directory.")
    print("="*50 + "\n")
    
    print("Top 5 product predictions:")
    for r in results[:5]:
        print(f"Product SKU: {r.sku}")
        print(f"  Demand Cluster (K-Means): {r.demand_label}")
        print(f"  Stock Status (LSVM): {r.stock_status} (Confidence: {r.confidence:.1f}%)")
        print(f"  Priority Level: {r.priority}")
        print(f"  EOQ: {r.eoq} units")
        print(f"  7-Day Demand Forecast: {r.forecast_7d:.2f} units")
        print(f"  Explanation: {r.explanations}\n")

if __name__ == "__main__":
    main()
