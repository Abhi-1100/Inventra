import sys
sys.path.append(r'C:\Inventra\build')
from backend.database.db_manager import DBManager
from backend.services.prediction_service import PredictionService
db = DBManager(r'C:\Inventra\build\inventra.db')
svc = PredictionService(db)
res = svc.run_predictions(lead_time_days=7, ordering_cost=20.0, holding_cost_rate=0.25, forecast_horizon=7)
print("Result count:", len(res))
