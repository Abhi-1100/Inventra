"""
Inventra Backend Integration Test
Run from repo root: python test_backend.py
"""

import sys
import os
import tempfile
import traceback

# Ensure repo root is on path
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

PASS = "OK"
FAIL = "FAIL"
results = []

def test(name, fn):
    try:
        fn()
        results.append((PASS, name))
        print(f"  [OK]   {name}")
    except Exception as e:
        results.append((FAIL, name))
        print(f"  [FAIL] {name}")
        traceback.print_exc()

# ─────────────────────────────────────────────
# 1. Imports
# ─────────────────────────────────────────────
print("\n" + "="*52)
print("  Inventra Backend Integration Test")
print("="*52)
print("\n[1/6] Imports\n")

def test_imports():
    from backend.database.db_manager import DBManager
    from backend.services.prediction_service import PredictionService
    from backend.services.analytics_service import AnalyticsService
    from backend.ml.prediction_engine import run_ml_pipeline
    from backend.ml.forecasting import forecast_demand
    from backend.ml.eoq import calculate_eoq
    from backend.models.entities import MLResult, Product, DailyEntry

test("All backend module imports", test_imports)

# ─────────────────────────────────────────────
# 2. Database
# ─────────────────────────────────────────────
print("\n[2/6] Database\n")

_tmp = tempfile.NamedTemporaryFile(suffix=".db", delete=False)
_tmp.close()
DB_PATH = _tmp.name

db = None

def test_db_init():
    global db
    from backend.database.db_manager import DBManager
    db = DBManager(DB_PATH)
    assert db is not None

def test_db_list_products_empty():
    products = db.list_products()
    assert isinstance(products, list)
    assert len(products) == 0, f"Expected 0 products, got {len(products)}"

def test_db_add_product():
    from backend.models.entities import Product
    p = Product(sku="TST-001", name="Test Rice 5kg", category="Staples",
                current_stock=100, unit_cost=250.0, reorder_point=20)
    new_id = db.add_product(p)
    assert new_id > 0, f"Expected positive ID, got {new_id}"

def test_db_list_products_after_add():
    products = db.list_products()
    assert len(products) == 1, f"Expected 1 product, got {len(products)}"
    assert products[0].sku == "TST-001"

def test_db_seed_sales():
    from backend.models.entities import DailyEntry
    import datetime
    products = db.list_products()
    pid = products[0].id
    today = datetime.date.today()
    for d in range(35, 0, -1):
        e = DailyEntry(
            product_id=pid,
            entry_date=(today - datetime.timedelta(days=d)).strftime("%Y-%m-%d"),
            units_sold=12 + (d % 5),
            units_wasted=1
        )
        db.add_daily_entry(e)
    with db.connection() as conn:
        c = conn.cursor()
        c.execute("SELECT COUNT(*) as cnt FROM daily_entries WHERE product_id=?", (pid,))
        cnt = c.fetchone()["cnt"]
    assert cnt == 35, f"Expected 35 entries, got {cnt}"

test("DBManager initialize", test_db_init)
test("list_products() on empty DB", test_db_list_products_empty)
test("add_product()", test_db_add_product)
test("list_products() after add", test_db_list_products_after_add)
test("save_daily_entry() x35 days", test_db_seed_sales)

# ─────────────────────────────────────────────
# 3. EOQ Calculation
# ─────────────────────────────────────────────
print("\n[3/6] EOQ Module\n")

def test_eoq_standard():
    from backend.ml.eoq import calculate_eoq
    result = calculate_eoq(demand=10.0, ordering_cost=50.0, holding_cost_rate=0.20, unit_cost=100.0)
    assert isinstance(result, int)
    assert 100 < result < 200, f"EOQ out of expected range: {result}"
    print(f"         EOQ(demand=10, S=50, H=20%, C=100) = {result} units")

def test_eoq_zero_cost():
    from backend.ml.eoq import calculate_eoq
    result = calculate_eoq(demand=10.0, ordering_cost=0.0, holding_cost_rate=0.25, unit_cost=100.0)
    assert result == 0, f"Expected 0 for zero ordering_cost, got {result}"

def test_eoq_zero_demand():
    from backend.ml.eoq import calculate_eoq
    result = calculate_eoq(demand=0.0, ordering_cost=20.0, holding_cost_rate=0.25, unit_cost=100.0)
    assert result == 0, f"Expected 0 for zero demand, got {result}"

test("EOQ standard Wilson formula", test_eoq_standard)
test("EOQ with zero ordering cost", test_eoq_zero_cost)
test("EOQ with zero demand", test_eoq_zero_demand)

# ─────────────────────────────────────────────
# 4. Forecasting
# ─────────────────────────────────────────────
print("\n[4/6] Forecasting Engine\n")

def test_forecast_demand():
    import pandas as pd
    import datetime
    from backend.ml.forecasting import forecast_demand

    today = datetime.date.today()
    dates = [(today - datetime.timedelta(days=d)).strftime("%Y-%m-%d") for d in range(35, 0, -1)]
    sales = [12 + (i % 5) for i in range(35)]
    df = pd.DataFrame({"date": dates, "sales_volume": sales})

    total, trend, points, model = forecast_demand(df, horizon=7)
    assert isinstance(total, float)
    assert total >= 0
    assert len(points) == 7, f"Expected 7 forecast points, got {len(points)}"
    assert all("yhat" in p for p in points)
    print(f"         Model used: {model}")
    print(f"         7-day total forecast: {total:.1f} units, trend: {trend:+.3f}/day")

test("forecast_demand() 7-day horizon", test_forecast_demand)

# ─────────────────────────────────────────────
# 5. Full ML Prediction Pipeline
# ─────────────────────────────────────────────
print("\n[5/6] ML Prediction Pipeline\n")

def test_prediction_pipeline():
    import pandas as pd
    import datetime
    from backend.ml.prediction_engine import run_ml_pipeline

    today = datetime.date.today()
    skus = ["SKU-A", "SKU-B"]
    products_list = [
        {"id": 1, "sku": "SKU-A", "name": "Atta 5kg", "category": "Staples",
         "current_stock": 15, "unit_cost": 250.0, "reorder_point": 10},
        {"id": 2, "sku": "SKU-B", "name": "Milk 1L", "category": "Dairy",
         "current_stock": 5, "unit_cost": 65.0, "reorder_point": 20},
    ]

    rows = []
    for sku, base in zip(skus, [12, 18]):
        for d in range(35, 0, -1):
            rows.append({
                "sku": sku, "name": "Product", "category": "Test",
                "sales_volume": base + (d % 4),
                "date": (today - datetime.timedelta(days=d)).strftime("%Y-%m-%d"),
                "product_id": 1 if sku == "SKU-A" else 2
            })

    df = pd.DataFrame(rows)
    results_list = run_ml_pipeline(
        sales_history_df=df,
        products_list=products_list,
        lead_time_days=7,
        ordering_cost=20.0,
        holding_cost_rate=0.25,
        forecast_horizon=7
    )

    assert len(results_list) > 0, "Pipeline returned no results"
    for r in results_list:
        assert r.sku in skus
        assert 0 <= r.confidence <= 100
        assert r.eoq >= 0
        assert r.demand_label in ("High", "Medium", "Low")
        assert r.priority in ("Critical", "ReorderSoon", "Safe")
        print(f"         {r.sku}: status={r.stock_status}, priority={r.priority}, "
              f"eoq={r.eoq}, forecast_7d={r.forecast_7d:.1f}, confidence={r.confidence:.1f}%")

def test_pipeline_with_db():
    from backend.services.prediction_service import PredictionService
    svc = PredictionService(db)
    results_list = svc.run_predictions(
        lead_time_days=7, ordering_cost=20.0,
        holding_cost_rate=0.25, forecast_horizon=7
    )
    assert len(results_list) == 1, f"Expected 1 result, got {len(results_list)}"
    r = results_list[0]
    print(f"         DB pipeline: {r.sku} -> status={r.stock_status}, "
          f"forecast_7d={r.forecast_7d:.1f}, eoq={r.eoq}")
    history = db.get_latest_pipeline_results(limit=1)
    assert len(history) > 0, "Pipeline result was not saved to DB"
    assert history[0]["success"] == 1

test("run_ml_pipeline() with synthetic data", test_prediction_pipeline)
test("PredictionService.run_predictions() from DB", test_pipeline_with_db)

# ─────────────────────────────────────────────
# 6. Analytics Service
# ─────────────────────────────────────────────
print("\n[6/6] Analytics Service\n")

def test_analytics_top_sellers():
    from backend.services.analytics_service import AnalyticsService
    svc = AnalyticsService(db)
    top = svc.get_top_selling_products(limit=5)
    assert isinstance(top, list)
    print(f"         Top sellers: {len(top)} returned")
    if top:
        print(f"         #1: {top[0].get('name','?')} - {top[0].get('total_sold','?')} units")

def test_analytics_categories():
    from backend.services.analytics_service import AnalyticsService
    svc = AnalyticsService(db)
    cats = svc.get_category_summaries()
    assert isinstance(cats, list)
    print(f"         Categories: {len(cats)}")

def test_analytics_forecast_trends():
    from backend.services.analytics_service import AnalyticsService
    svc = AnalyticsService(db)
    trends = svc.get_forecast_trends()
    assert isinstance(trends, list)
    print(f"         Forecast trends: {len(trends)} items")

test("AnalyticsService.get_top_selling_products()", test_analytics_top_sellers)
test("AnalyticsService.get_category_summaries()", test_analytics_categories)
test("AnalyticsService.get_forecast_trends()", test_analytics_forecast_trends)

# ─────────────────────────────────────────────
# Summary
# ─────────────────────────────────────────────
try:
    os.unlink(DB_PATH)
except:
    pass

print("\n" + "="*52)
passed = sum(1 for r in results if r[0] == PASS)
failed = sum(1 for r in results if r[0] == FAIL)
print(f"  Results: {passed} passed, {failed} failed out of {len(results)} tests")
print("="*52)
if failed:
    print("\n  Failed tests:")
    for status, name in results:
        if status == FAIL:
            print(f"    [FAIL] {name}")
    sys.exit(1)
else:
    print("\n  All backend tests passed!")
    sys.exit(0)
