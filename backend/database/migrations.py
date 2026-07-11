"""
Database Migration Runner for the AI Inventory Management Backend.
Creates the schema and tables if they do not exist.
Matches the existing schema from the C++ desktop application.
"""

import sqlite3
from backend.utils.logger import get_logger

logger = get_logger("migrations")

SCHEMA_VERSION = 1

DDL_STATEMENTS = [
    # Schema version
    """
    CREATE TABLE IF NOT EXISTS schema_version (
        version INTEGER PRIMARY KEY
    );
    """,
    # Shop profile
    """
    CREATE TABLE IF NOT EXISTS shop (
        id         INTEGER PRIMARY KEY,
        name       TEXT NOT NULL DEFAULT '',
        owner_name TEXT NOT NULL DEFAULT '',
        phone      TEXT NOT NULL DEFAULT '',
        logo_path  TEXT DEFAULT ''
    );
    """,
    # Staff users
    """
    CREATE TABLE IF NOT EXISTS users (
        id         INTEGER PRIMARY KEY AUTOINCREMENT,
        name       TEXT    NOT NULL,
        role       TEXT    NOT NULL CHECK(role IN ('Owner','Staff')),
        pin_hash   TEXT    NOT NULL,
        created_at TEXT    DEFAULT (datetime('now'))
    );
    """,
    # Product catalog
    """
    CREATE TABLE IF NOT EXISTS products (
        id            INTEGER PRIMARY KEY AUTOINCREMENT,
        sku           TEXT    UNIQUE NOT NULL,
        name          TEXT    NOT NULL,
        category      TEXT    DEFAULT '',
        current_stock INTEGER DEFAULT 0,
        unit_cost     REAL    DEFAULT 0,
        reorder_point INTEGER DEFAULT 10,
        created_at    TEXT    DEFAULT (datetime('now')),
        updated_at    TEXT    DEFAULT (datetime('now'))
    );
    """,
    # Daily sales + waste entries per product per day
    """
    CREATE TABLE IF NOT EXISTS daily_entries (
        id           INTEGER PRIMARY KEY AUTOINCREMENT,
        product_id   INTEGER REFERENCES products(id) ON DELETE CASCADE,
        entry_date   TEXT    NOT NULL,
        units_sold   INTEGER DEFAULT 0,
        units_wasted INTEGER DEFAULT 0,
        entered_by   INTEGER REFERENCES users(id) ON DELETE SET NULL,
        created_at   TEXT    DEFAULT (datetime('now'))
    );
    """,
    # Stock movement ledger
    """
    CREATE TABLE IF NOT EXISTS stock_movements (
        id              INTEGER PRIMARY KEY AUTOINCREMENT,
        product_id      INTEGER REFERENCES products(id) ON DELETE CASCADE,
        movement_type   TEXT    NOT NULL CHECK(movement_type IN ('IN','OUT')),
        quantity        INTEGER NOT NULL,
        supplier_name   TEXT    DEFAULT '',
        cost_per_unit   REAL    DEFAULT 0,
        reason          TEXT    DEFAULT '',
        movement_date   TEXT    NOT NULL,
        entered_by      INTEGER REFERENCES users(id) ON DELETE SET NULL,
        created_at      TEXT    DEFAULT (datetime('now'))
    );
    """,
    # ML pipeline results history
    """
    CREATE TABLE IF NOT EXISTS pipeline_results (
        id           INTEGER PRIMARY KEY AUTOINCREMENT,
        run_at       TEXT    NOT NULL,
        success      INTEGER DEFAULT 1,
        results_json TEXT    DEFAULT ''
    );
    """,
    # App settings (key-value)
    """
    CREATE TABLE IF NOT EXISTS app_settings (
        key   TEXT PRIMARY KEY,
        value TEXT
    );
    """
]

def run_migrations(conn: sqlite3.Connection) -> None:
    """
    Runs DDL schema creation statements if tables do not exist.
    """
    cursor = conn.cursor()
    try:
        # Check current schema version
        cursor.execute("SELECT name FROM sqlite_master WHERE type='table' AND name='schema_version';")
        has_version_table = cursor.fetchone()
        
        current_version = 0
        if has_version_table:
            cursor.execute("SELECT version FROM schema_version LIMIT 1;")
            row = cursor.fetchone()
            if row:
                current_version = row[0]
                
        if current_version >= SCHEMA_VERSION:
            logger.debug("Database schema is up to date.")
            return

        logger.info(f"Running database migrations (current={current_version}, target={SCHEMA_VERSION})...")
        
        # Execute each DDL statement
        for statement in DDL_STATEMENTS:
            cursor.execute(statement)
            
        # Update schema version
        cursor.execute("INSERT OR REPLACE INTO schema_version(version) VALUES(?);", (SCHEMA_VERSION,))
        conn.commit()
        logger.info("Database migrations completed successfully.")
        
    except Exception as e:
        conn.rollback()
        logger.error(f"Migration failure: {e}", exc_info=True)
        raise RuntimeError(f"Failed to run database migrations: {e}") from e
