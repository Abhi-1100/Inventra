"""
Unit Tests for Database Manager and Migrations.
Tests connection parameters, transaction states, and complete database CRUD.
"""

import os
import json
import pytest
import sqlite3
from backend.database.db_manager import DBManager
from backend.models.entities import Product, DailyEntry, StockMovement, ShopProfile, StaffUser
from backend.utils.validators import DatabaseError

@pytest.fixture
def temp_db(tmp_path):
    """Fixture to create a temporary test database file."""
    db_file = tmp_path / "test_inventory.db"
    return str(db_file)

def test_db_initialization_and_migrations(temp_db):
    """Verifies that DBManager successfully creates all tables on init."""
    db = DBManager(temp_db)
    
    # Assert connection works and tables exist
    with db.connection() as conn:
        cursor = conn.cursor()
        cursor.execute("SELECT name FROM sqlite_master WHERE type='table';")
        tables = [row["name"] for row in cursor.fetchall()]
        
        expected_tables = [
            "schema_version", "shop", "users", "products", 
            "daily_entries", "stock_movements", "pipeline_results", "app_settings"
        ]
        for t in expected_tables:
            assert t in tables

def test_transaction_rollback(temp_db):
    """Verifies transaction context manager rolls back on exception."""
    db = DBManager(temp_db)
    
    # Add a product to work with
    p = Product(sku="TEST-01", name="Test Item", category="Cat", current_stock=10, unit_cost=5.0)
    p_id = db.add_product(p)
    
    # Attempt stock movement and fail halfway
    with pytest.raises(DatabaseError):
        with db.transaction() as cursor:
            # 1. Insert valid movement
            cursor.execute("""
                INSERT INTO stock_movements (product_id, movement_type, quantity, supplier_name, cost_per_unit, reason, movement_date, entered_by, created_at)
                VALUES (?, 'IN', 5, 'Supplier', 5.0, 'Reason', '2026-07-11', NULL, datetime('now'));
            """, (p_id,))
            
            # 2. Trigger IntegrityError via bad product_id reference
            cursor.execute("""
                INSERT INTO stock_movements (product_id, movement_type, quantity, supplier_name, cost_per_unit, reason, movement_date, entered_by, created_at)
                VALUES (999, 'IN', 5, 'Supplier', 5.0, 'Reason', '2026-07-11', NULL, datetime('now'));
            """)
            
    # Check that the first valid movement was NOT committed (rolled back)
    with db.connection() as conn:
        cursor = conn.cursor()
        cursor.execute("SELECT COUNT(*) FROM stock_movements;")
        count = cursor.fetchone()[0]
        assert count == 0
        
        # Check stock level is still 10 (not updated to 15)
        cursor.execute("SELECT current_stock FROM products WHERE id = ?;", (p_id,))
        stock = cursor.fetchone()[0]
        assert stock == 10

def test_product_crud(temp_db):
    """Tests product ADD, GET, UPDATE, and DELETE queries."""
    db = DBManager(temp_db)
    
    p = Product(sku="SKU-1", name="Apple", category="Fruit", current_stock=50, unit_cost=1.2, reorder_point=15)
    
    # 1. ADD
    p_id = db.add_product(p)
    assert p_id > 0
    
    # 2. GET
    fetched = db.get_product(p_id)
    assert fetched is not None
    assert fetched.sku == "SKU-1"
    assert fetched.name == "Apple"
    
    fetched_sku = db.get_product_by_sku("SKU-1")
    assert fetched_sku is not None
    assert fetched_sku.id == p_id
    
    # 3. UPDATE
    fetched.name = "Green Apple"
    fetched.current_stock = 60
    assert db.update_product(fetched)
    
    updated = db.get_product(p_id)
    assert updated.name == "Green Apple"
    assert updated.current_stock == 60
    
    # 4. LIST
    products = db.list_products()
    assert len(products) == 1
    assert products[0].sku == "SKU-1"
    
    # 5. DELETE
    assert db.delete_product(p_id)
    assert db.get_product(p_id) is None

def test_daily_entries_crud(temp_db):
    """Tests CRUD operations for daily entries."""
    db = DBManager(temp_db)
    p_id = db.add_product(Product(sku="P1", name="Prod 1"))
    
    # Add
    entry = DailyEntry(product_id=p_id, entry_date="2026-07-11", units_sold=5, units_wasted=2)
    entry_id = db.add_daily_entry(entry)
    assert entry_id > 0
    
    # Get range
    entries = db.get_daily_entries_range("2026-07-10", "2026-07-12")
    assert len(entries) == 1
    assert entries[0].id == entry_id
    assert entries[0].units_sold == 5
    assert entries[0].units_wasted == 2
    
    # Get for product
    prod_entries = db.get_daily_entries_for_product(p_id)
    assert len(prod_entries) == 1
    
    # Update
    entry.id = entry_id
    entry.units_sold = 10
    entry.units_wasted = 1
    assert db.update_daily_entry(entry)
    
    updated_entries = db.get_daily_entries_for_product(p_id)
    assert updated_entries[0].units_sold == 10
    assert updated_entries[0].units_wasted == 1
    
    # Delete
    assert db.delete_daily_entry(entry_id)
    assert len(db.get_daily_entries_for_product(p_id)) == 0

def test_stock_movements_and_settings(temp_db):
    """Tests stock movements recording and app settings key-value store."""
    db = DBManager(temp_db)
    p_id = db.add_product(Product(sku="P1", name="Prod 1", current_stock=20))
    
    # Stock movement OUT
    m = StockMovement(product_id=p_id, movement_type="OUT", quantity=5, movement_date="2026-07-11")
    m_id = db.add_stock_movement(m)
    assert m_id > 0
    
    # Check that current stock decreased to 15
    assert db.get_product(p_id).current_stock == 15
    
    # Check range and product retrieval
    movs = db.get_stock_movements_range("2026-07-10", "2026-07-12")
    assert len(movs) == 1
    assert movs[0].quantity == 5
    
    movs_p = db.get_stock_movements_for_product(p_id)
    assert len(movs_p) == 1
    
    # Settings key-value
    assert db.set_setting("key1", "val1")
    assert db.get_setting("key1") == "val1"
    assert db.get_setting("nonexistent", "default") == "default"

def test_users_and_shop_and_pipeline(temp_db):
    """Tests users, shop profile, and pipeline results CRUD."""
    db = DBManager(temp_db)
    
    # User creation
    u_id = db.create_user("Alice", "Staff", "12345")
    assert u_id > 0
    users = db.get_users()
    assert len(users) == 1
    assert users[0].name == "Alice"
    
    # Shop profile
    shop = ShopProfile(name="My Shop", owner_name="Bob", phone="12345", logo_path="logo.png")
    assert db.save_shop(shop)
    fetched_shop = db.get_shop()
    assert fetched_shop is not None
    assert fetched_shop.name == "My Shop"
    assert fetched_shop.owner_name == "Bob"
    
    # Pipeline result
    res_id = db.save_pipeline_result("2026-07-11 10:00:00", True, json.dumps([{"sku": "SKU1", "forecast": 10.0}]))
    assert res_id > 0
    latest = db.get_latest_pipeline_results()
    assert len(latest) == 1
    assert latest[0]["id"] == res_id
    assert latest[0]["success"] == 1

def test_db_execute_error(temp_db):
    """Tests that executing invalid queries raises DatabaseError wrapper."""
    db = DBManager(temp_db)
    with pytest.raises(DatabaseError):
        with db.connection() as conn:
            cursor = conn.cursor()
            cursor.execute("SELECT * FROM nonexistent_table;")

def test_migrations_already_up_to_date(temp_db):
    """Tests that running migrations on an already up to date db exits early."""
    from backend.database.migrations import run_migrations
    db = DBManager(temp_db)
    with db.connection() as conn:
        # Run migrations a second time manually (should be no-op and succeed)
        run_migrations(conn)

def test_db_connection_failure():
    """Tests that initializing a DBManager with an invalid path raises DatabaseError."""
    # An invalid Windows path containing invalid characters like * or ?
    invalid_path = "C:\\invalid_?_path\\inventory.db"
    with pytest.raises(DatabaseError):
        DBManager(invalid_path)
