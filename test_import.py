import sys
sys.path.append(r'C:\Inventra\build')
from backend.database.db_manager import DBManager
from backend.services.import_service import ImportService
db = DBManager(r'C:\Inventra\build\inventra.db')
svc = ImportService(db)
svc.import_csv(r'C:\Inventra\sample_inventory.csv')
