"""
Database Manager for the AI Inventory Management Backend.
Handles connection lifecycles, transaction states, and SQL execution.
Shields application layers from raw sqlite3 operations.
"""

import sqlite3
import json
from contextlib import contextmanager
from typing import List, Optional, Dict, Any, Generator

from backend.config import settings
from backend.models.entities import Product, DailyEntry, StockMovement, ShopProfile, StaffUser
from backend.database.migrations import run_migrations
from backend.utils.logger import get_logger
from backend.utils.validators import DatabaseError

logger = get_logger("db_manager")

class DBManager:
    """
    Manages SQLite connection lifecycle, transactions, and CRUD mapping.
    """
    def __init__(self, db_path: Optional[str] = None):
        self.db_path = db_path or str(settings.DEFAULT_DB_PATH)
        self._initialize_db()

    def _initialize_db(self) -> None:
        """Initializes database connection and applies migrations."""
        try:
            with self.connection() as conn:
                run_migrations(conn)
        except Exception as e:
            logger.critical(f"Database initialization failed: {e}", exc_info=True)
            raise DatabaseError(f"Database initialization failed: {e}") from e

    @contextmanager
    def connection(self) -> Generator[sqlite3.Connection, None, None]:
        """
        Context manager for obtaining a database connection.
        Configures row factory, WAL, synchronous write levels, and foreign key settings.
        """
        conn = None
        try:
            conn = sqlite3.connect(self.db_path)
            conn.row_factory = sqlite3.Row
            
            # Performance & Constraint Pragmas
            cursor = conn.cursor()
            cursor.execute("PRAGMA journal_mode=WAL;")
            cursor.execute("PRAGMA foreign_keys=ON;")
            cursor.execute("PRAGMA synchronous=NORMAL;")
            
            yield conn
            
        except Exception as e:
            from backend.utils.validators import ValidationError
            if isinstance(e, ValidationError):
                raise e
            logger.error(f"Database connection error: {e}", exc_info=True)
            if conn:
                try:
                    conn.rollback()
                except Exception as rollback_err:
                    logger.error(f"Rollback error: {rollback_err}")
            raise DatabaseError(f"Database error: {e}") from e
        finally:
            if conn:
                conn.close()

    @contextmanager
    def transaction(self) -> Generator[sqlite3.Cursor, None, None]:
        """
        Context manager for executing statements inside a transaction block.
        Automatically commits or rolls back on exceptions.
        """
        with self.connection() as conn:
            cursor = conn.cursor()
            try:
                yield cursor
                conn.commit()
            except Exception as e:
                conn.rollback()
                logger.error(f"Transaction failed and was rolled back: {e}", exc_info=True)
                from backend.utils.validators import ValidationError
                if isinstance(e, ValidationError):
                    raise e
                raise DatabaseError(f"Database transaction failure: {e}") from e

    # ─────────────────────────────────────────────
    # Product CRUD
    # ─────────────────────────────────────────────

    def add_product(self, product: Product) -> int:
        """
        Adds a new product to the catalog.
        Returns the auto-generated product ID.
        """
        query = """
        INSERT INTO products (sku, name, category, current_stock, unit_cost, reorder_point, created_at, updated_at)
        VALUES (?, ?, ?, ?, ?, ?, datetime('now'), datetime('now'));
        """
        try:
            with self.transaction() as cursor:
                cursor.execute(query, (
                    product.sku,
                    product.name,
                    product.category,
                    product.current_stock,
                    product.unit_cost,
                    product.reorder_point
                ))
                product_id = cursor.lastrowid
                logger.info(f"Added product SKU={product.sku} with ID={product_id}")
                return product_id
        except sqlite3.IntegrityError as e:
            raise DatabaseError(f"Integrity check failed. Possibly duplicate SKU or missing fields: {e}")
        except Exception as e:
            raise DatabaseError(f"Failed to add product: {e}")

    def update_product(self, product: Product) -> bool:
        """
        Updates an existing product in the catalog.
        """
        query = """
        UPDATE products
        SET sku = ?, name = ?, category = ?, current_stock = ?, unit_cost = ?, reorder_point = ?, updated_at = datetime('now')
        WHERE id = ?;
        """
        try:
            with self.transaction() as cursor:
                cursor.execute(query, (
                    product.sku,
                    product.name,
                    product.category,
                    product.current_stock,
                    product.unit_cost,
                    product.reorder_point,
                    product.id
                ))
                updated = cursor.rowcount > 0
                logger.info(f"Updated product ID={product.id}, status={updated}")
                return updated
        except sqlite3.IntegrityError as e:
            raise DatabaseError(f"Integrity check failed: {e}")
        except Exception as e:
            raise DatabaseError(f"Failed to update product ID={product.id}: {e}")

    def delete_product(self, product_id: int) -> bool:
        """
        Deletes a product from the database (hard delete).
        Note: cascades to delete daily_entries and stock_movements.
        """
        query = "DELETE FROM products WHERE id = ?;"
        try:
            with self.transaction() as cursor:
                cursor.execute(query, (product_id,))
                deleted = cursor.rowcount > 0
                logger.info(f"Deleted product ID={product_id}, status={deleted}")
                return deleted
        except Exception as e:
            raise DatabaseError(f"Failed to delete product ID={product_id}: {e}")

    def get_product(self, product_id: int) -> Optional[Product]:
        """
        Retrieves a single product by ID.
        """
        query = "SELECT * FROM products WHERE id = ?;"
        try:
            with self.connection() as conn:
                cursor = conn.cursor()
                cursor.execute(query, (product_id,))
                row = cursor.fetchone()
                if row:
                    return Product(**dict(row))
                return None
        except Exception as e:
            raise DatabaseError(f"Failed to fetch product ID={product_id}: {e}")

    def get_product_by_sku(self, sku: str) -> Optional[Product]:
        """
        Retrieves a single product by SKU.
        """
        query = "SELECT * FROM products WHERE sku = ?;"
        try:
            with self.connection() as conn:
                cursor = conn.cursor()
                cursor.execute(query, (sku,))
                row = cursor.fetchone()
                if row:
                    return Product(**dict(row))
                return None
        except Exception as e:
            raise DatabaseError(f"Failed to fetch product SKU={sku}: {e}")

    def list_products(self) -> List[Product]:
        """
        Retrieves all products from the catalog.
        """
        query = "SELECT * FROM products ORDER BY name ASC;"
        try:
            with self.connection() as conn:
                cursor = conn.cursor()
                cursor.execute(query)
                rows = cursor.fetchall()
                return [Product(**dict(row)) for row in rows]
        except Exception as e:
            raise DatabaseError(f"Failed to list products: {e}")

    # ─────────────────────────────────────────────
    # Daily Entries CRUD
    # ─────────────────────────────────────────────

    def add_daily_entry(self, entry: DailyEntry) -> int:
        """
        Records a daily sales/waste entry.
        """
        query = """
        INSERT INTO daily_entries (product_id, entry_date, units_sold, units_wasted, entered_by, created_at)
        VALUES (?, ?, ?, ?, ?, datetime('now'));
        """
        try:
            with self.transaction() as cursor:
                cursor.execute(query, (
                    entry.product_id,
                    entry.entry_date,
                    entry.units_sold,
                    entry.units_wasted,
                    entry.entered_by
                ))
                entry_id = cursor.lastrowid
                logger.info(f"Added daily entry ID={entry_id} for product ID={entry.product_id}")
                return entry_id
        except sqlite3.IntegrityError as e:
            raise DatabaseError(f"Integrity check failed: {e}")
        except Exception as e:
            raise DatabaseError(f"Failed to add daily entry: {e}")

    def update_daily_entry(self, entry: DailyEntry) -> bool:
        """
        Updates an existing daily entry.
        """
        query = """
        UPDATE daily_entries
        SET units_sold = ?, units_wasted = ?
        WHERE id = ?;
        """
        try:
            with self.transaction() as cursor:
                cursor.execute(query, (
                    entry.units_sold,
                    entry.units_wasted,
                    entry.id
                ))
                updated = cursor.rowcount > 0
                logger.info(f"Updated daily entry ID={entry.id}, status={updated}")
                return updated
        except Exception as e:
            raise DatabaseError(f"Failed to update daily entry ID={entry.id}: {e}")

    def delete_daily_entry(self, entry_id: int) -> bool:
        """
        Deletes a daily sales/waste entry.
        """
        query = "DELETE FROM daily_entries WHERE id = ?;"
        try:
            with self.transaction() as cursor:
                cursor.execute(query, (entry_id,))
                deleted = cursor.rowcount > 0
                logger.info(f"Deleted daily entry ID={entry_id}, status={deleted}")
                return deleted
        except Exception as e:
            raise DatabaseError(f"Failed to delete daily entry ID={entry_id}: {e}")

    def get_daily_entries_range(self, from_date: str, to_date: str) -> List[DailyEntry]:
        """
        Retrieves daily entries within a date range (inclusive).
        """
        query = """
        SELECT de.*, p.name as product_name, p.sku as product_sku
        FROM daily_entries de
        LEFT JOIN products p ON p.id = de.product_id
        WHERE de.entry_date BETWEEN ? AND ?
        ORDER BY de.entry_date DESC, de.created_at DESC;
        """
        try:
            with self.connection() as conn:
                cursor = conn.cursor()
                cursor.execute(query, (from_date, to_date))
                rows = cursor.fetchall()
                return [DailyEntry(**dict(row)) for row in rows]
        except Exception as e:
            raise DatabaseError(f"Failed to fetch daily entries range: {e}")

    def get_daily_entries_for_product(self, product_id: int) -> List[DailyEntry]:
        """
        Retrieves all daily entries for a single product.
        """
        query = """
        SELECT de.*, p.name as product_name, p.sku as product_sku
        FROM daily_entries de
        LEFT JOIN products p ON p.id = de.product_id
        WHERE de.product_id = ?
        ORDER BY de.entry_date ASC;
        """
        try:
            with self.connection() as conn:
                cursor = conn.cursor()
                cursor.execute(query, (product_id,))
                rows = cursor.fetchall()
                return [DailyEntry(**dict(row)) for row in rows]
        except Exception as e:
            raise DatabaseError(f"Failed to fetch daily entries for product ID={product_id}: {e}")

    # ─────────────────────────────────────────────
    # Stock Movements CRUD
    # ─────────────────────────────────────────────

    def add_stock_movement(self, movement: StockMovement) -> int:
        """
        Records a stock movement (IN/OUT) and updates current_stock in products table.
        Runs inside a transaction block to maintain consistency.
        """
        insert_query = """
        INSERT INTO stock_movements (product_id, movement_type, quantity, supplier_name, cost_per_unit, reason, movement_date, entered_by, created_at)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, datetime('now'));
        """
        update_stock_query = """
        UPDATE products
        SET current_stock = current_stock + ?
        WHERE id = ?;
        """
        try:
            with self.transaction() as cursor:
                # 1. Insert movement ledger record
                cursor.execute(insert_query, (
                    movement.product_id,
                    movement.movement_type,
                    movement.quantity,
                    movement.supplier_name,
                    movement.cost_per_unit,
                    movement.reason,
                    movement.movement_date,
                    movement.entered_by
                ))
                movement_id = cursor.lastrowid

                # 2. Update stock levels (IN increases, OUT decreases)
                delta = movement.quantity if movement.movement_type == "IN" else -movement.quantity
                cursor.execute(update_stock_query, (delta, movement.product_id))

                logger.info(f"Recorded movement ID={movement_id} ({movement.movement_type}), delta={delta} for product ID={movement.product_id}")
                return movement_id
        except sqlite3.IntegrityError as e:
            raise DatabaseError(f"Integrity violation during stock movement: {e}")
        except Exception as e:
            raise DatabaseError(f"Failed to record stock movement: {e}")

    def get_stock_movements_range(self, from_date: str, to_date: str, type_filter: Optional[str] = None) -> List[StockMovement]:
        """
        Retrieves stock movements ledger within a date range (inclusive).
        """
        if type_filter:
            query = """
            SELECT sm.*, p.name as product_name, u.name as staff_name
            FROM stock_movements sm
            LEFT JOIN products p ON p.id = sm.product_id
            LEFT JOIN users u ON u.id = sm.entered_by
            WHERE sm.movement_date BETWEEN ? AND ? AND sm.movement_type = ?
            ORDER BY sm.movement_date DESC, sm.created_at DESC;
            """
            params = (from_date, to_date, type_filter)
        else:
            query = """
            SELECT sm.*, p.name as product_name, u.name as staff_name
            FROM stock_movements sm
            LEFT JOIN products p ON p.id = sm.product_id
            LEFT JOIN users u ON u.id = sm.entered_by
            WHERE sm.movement_date BETWEEN ? AND ?
            ORDER BY sm.movement_date DESC, sm.created_at DESC;
            """
            params = (from_date, to_date)

        try:
            with self.connection() as conn:
                cursor = conn.cursor()
                cursor.execute(query, params)
                rows = cursor.fetchall()
                return [StockMovement(**dict(row)) for row in rows]
        except Exception as e:
            raise DatabaseError(f"Failed to fetch stock movements range: {e}")

    def get_stock_movements_for_product(self, product_id: int, from_date: Optional[str] = None) -> List[StockMovement]:
        """
        Retrieves stock movements for a specific product.
        """
        if from_date:
            query = """
            SELECT sm.*, p.name as product_name, u.name as staff_name
            FROM stock_movements sm
            LEFT JOIN products p ON p.id = sm.product_id
            LEFT JOIN users u ON u.id = sm.entered_by
            WHERE sm.product_id = ? AND sm.movement_date >= ?
            ORDER BY sm.movement_date ASC;
            """
            params = (product_id, from_date)
        else:
            query = """
            SELECT sm.*, p.name as product_name, u.name as staff_name
            FROM stock_movements sm
            LEFT JOIN products p ON p.id = sm.product_id
            LEFT JOIN users u ON u.id = sm.entered_by
            WHERE sm.product_id = ?
            ORDER BY sm.movement_date ASC;
            """
            params = (product_id,)

        try:
            with self.connection() as conn:
                cursor = conn.cursor()
                cursor.execute(query, params)
                rows = cursor.fetchall()
                return [StockMovement(**dict(row)) for row in rows]
        except Exception as e:
            raise DatabaseError(f"Failed to fetch stock movements for product ID={product_id}: {e}")

    # ─────────────────────────────────────────────
    # App Settings (Key-Value)
    # ─────────────────────────────────────────────

    def set_setting(self, key: str, value: str) -> bool:
        """
        Sets or updates an app setting key-value pair.
        """
        query = "INSERT OR REPLACE INTO app_settings (key, value) VALUES (?, ?);"
        try:
            with self.transaction() as cursor:
                cursor.execute(query, (key, value))
                return True
        except Exception as e:
            raise DatabaseError(f"Failed to set setting '{key}': {e}")

    def get_setting(self, key: str, default: Optional[str] = None) -> Optional[str]:
        """
        Gets an app setting value. Returns default if key does not exist.
        """
        query = "SELECT value FROM app_settings WHERE key = ?;"
        try:
            with self.connection() as conn:
                cursor = conn.cursor()
                cursor.execute(query, (key,))
                row = cursor.fetchone()
                if row:
                    return row[0]
                return default
        except Exception as e:
            raise DatabaseError(f"Failed to fetch setting '{key}': {e}")

    # ─────────────────────────────────────────────
    # Pipeline Results
    # ─────────────────────────────────────────────

    def save_pipeline_result(self, run_at: str, success: bool, results_json: str) -> int:
        """
        Saves a record of an ML pipeline run.
        """
        query = """
        INSERT INTO pipeline_results (run_at, success, results_json)
        VALUES (?, ?, ?);
        """
        try:
            with self.transaction() as cursor:
                cursor.execute(query, (run_at, 1 if success else 0, results_json))
                return cursor.lastrowid
        except Exception as e:
            raise DatabaseError(f"Failed to save pipeline result: {e}")

    def get_latest_pipeline_results(self, limit: int = 10) -> List[Dict[str, Any]]:
        """
        Retrieves historical ML pipeline run records.
        """
        query = "SELECT * FROM pipeline_results ORDER BY run_at DESC LIMIT ?;"
        try:
            with self.connection() as conn:
                cursor = conn.cursor()
                cursor.execute(query, (limit,))
                rows = cursor.fetchall()
                return [dict(row) for row in rows]
        except Exception as e:
            raise DatabaseError(f"Failed to get pipeline results: {e}")

    # ─────────────────────────────────────────────
    # Users & Shop CRUD (for completeness / testing)
    # ─────────────────────────────────────────────

    def get_users(self) -> List[StaffUser]:
        """Retrieves all users."""
        query = "SELECT * FROM users ORDER BY role DESC, name ASC;"
        try:
            with self.connection() as conn:
                cursor = conn.cursor()
                cursor.execute(query)
                return [StaffUser(**dict(row)) for row in cursor.fetchall()]
        except Exception as e:
            raise DatabaseError(f"Failed to retrieve users: {e}")

    def create_user(self, name: str, role: str, pin_hash: str) -> int:
        """Creates a user."""
        query = "INSERT INTO users (name, role, pin_hash, created_at) VALUES (?, ?, ?, datetime('now'));"
        try:
            with self.transaction() as cursor:
                cursor.execute(query, (name, role, pin_hash))
                return cursor.lastrowid
        except Exception as e:
            raise DatabaseError(f"Failed to create user: {e}")

    def get_shop(self) -> Optional[ShopProfile]:
        """Gets the single shop profile row."""
        query = "SELECT * FROM shop LIMIT 1;"
        try:
            with self.connection() as conn:
                cursor = conn.cursor()
                cursor.execute(query)
                row = cursor.fetchone()
                return ShopProfile(**dict(row)) if row else None
        except Exception as e:
            raise DatabaseError(f"Failed to fetch shop: {e}")

    def save_shop(self, shop: ShopProfile) -> bool:
        """Saves/updates the shop profile."""
        query = """
        INSERT INTO shop (id, name, owner_name, phone, logo_path)
        VALUES (1, ?, ?, ?, ?)
        ON CONFLICT(id) DO UPDATE SET
            name=excluded.name,
            owner_name=excluded.owner_name,
            phone=excluded.phone,
            logo_path=excluded.logo_path;
        """
        try:
            with self.transaction() as cursor:
                cursor.execute(query, (shop.name, shop.owner_name, shop.phone, shop.logo_path))
                return True
        except Exception as e:
            raise DatabaseError(f"Failed to save shop profile: {e}")
