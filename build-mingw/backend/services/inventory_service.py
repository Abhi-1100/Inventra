"""
Inventory Service for the AI Inventory Management Backend.
Orchestrates product CRUD, bulk CSV import, stock ledger entries, deactivations,
and automated weekly sales calculations.
"""

import os
import json
import pandas as pd
from datetime import datetime
from typing import List, Dict, Any, Optional

from backend.database.db_manager import DBManager
from backend.models.entities import Product, DailyEntry, StockMovement
from backend.utils.logger import get_logger
from backend.utils.validators import (
    validate_product_data,
    validate_stock_movement_data,
    validate_date_format,
    validate_non_negative,
    ValidationError,
    DuplicateProductError
)

logger = get_logger("inventory_service")

class InventoryService:
    """
    Handles business logic for product management and inventory tracking.
    """
    def __init__(self, db_manager: DBManager):
        self.db = db_manager

    # ─────────────────────────────────────────────
    # Product Management
    # ─────────────────────────────────────────────

    def add_product(self, product: Product) -> int:
        """
        Adds a product to the database. Validates input and detects duplicates.
        """
        validate_product_data(
            product.sku,
            product.name,
            product.unit_cost,
            product.current_stock,
            product.reorder_point
        )
        
        # Duplicate detection (sku and name)
        existing_sku = self.db.get_product_by_sku(product.sku)
        if existing_sku:
            logger.warning(f"Failed to add product: SKU '{product.sku}' already exists.")
            raise DuplicateProductError(f"Product with SKU '{product.sku}' already exists.")
            
        # Check name duplicates
        all_products = self.db.list_products()
        for p in all_products:
            if p.name.strip().lower() == product.name.strip().lower():
                logger.warning(f"Failed to add product: Name '{product.name}' already exists.")
                raise DuplicateProductError(f"Product with Name '{product.name}' already exists.")
                
        return self.db.add_product(product)

    def update_product(self, product: Product) -> bool:
        """
        Updates product attributes in the database.
        """
        validate_product_data(
            product.sku,
            product.name,
            product.unit_cost,
            product.current_stock,
            product.reorder_point
        )
        
        # Verify product exists
        existing = self.db.get_product(product.id)
        if not existing:
            raise ValidationError(f"Product with ID={product.id} does not exist.")
            
        # Check for SKU duplicate (if SKU changed)
        if existing.sku != product.sku:
            dup_sku = self.db.get_product_by_sku(product.sku)
            if dup_sku:
                raise DuplicateProductError(f"Product with SKU '{product.sku}' already exists.")
                
        # Check for Name duplicate (if Name changed)
        if existing.name.strip().lower() != product.name.strip().lower():
            for p in self.db.list_products():
                if p.id != product.id and p.name.strip().lower() == product.name.strip().lower():
                    raise DuplicateProductError(f"Product with Name '{product.name}' already exists.")
                    
        return self.db.update_product(product)

    def deactivate_product(self, product_id: int) -> bool:
        """
        Logically deactivates a product by storing its SKU in settings.
        Prevents deletion from cascading and destroying historical logs.
        """
        product = self.db.get_product(product_id)
        if not product:
            logger.warning(f"Attempted deactivation of non-existent product ID={product_id}")
            raise ValidationError(f"Product with ID={product_id} does not exist.")
            
        try:
            deactivated_json = self.db.get_setting("deactivated_skus", "[]")
            deactivated_list = json.loads(deactivated_json)
            
            if product.sku not in deactivated_list:
                deactivated_list.append(product.sku)
                self.db.set_setting("deactivated_skus", json.dumps(deactivated_list))
                logger.info(f"Logically deactivated product SKU={product.sku}")
                return True
                
            return False
        except Exception as e:
            logger.error(f"Deactivation failed for ID={product_id}: {e}", exc_info=True)
            raise

    def is_active(self, sku: str) -> bool:
        """Checks if a product is active (not deactivated)."""
        deactivated_json = self.db.get_setting("deactivated_skus", "[]")
        deactivated_list = json.loads(deactivated_json)
        return sku not in deactivated_list

    def list_active_products(self) -> List[Product]:
        """Returns all products that are currently active."""
        all_products = self.db.list_products()
        deactivated_json = self.db.get_setting("deactivated_skus", "[]")
        deactivated_list = json.loads(deactivated_json)
        return [p for p in all_products if p.sku not in deactivated_list]

    def bulk_csv_import(self, file_path: str) -> Dict[str, Any]:
        """
        Imports products from a CSV file.
        Validates columns, structures data types, and inserts in batches.
        """
        if not os.path.exists(file_path):
            raise ValidationError(f"CSV file not found at path: {file_path}")
            
        try:
            df = pd.read_csv(file_path)
            # Standardize column headers
            df.columns = [c.strip().lower().replace(" ", "_") for c in df.columns]
            
            # Check required columns
            required = ['sku', 'name', 'current_stock', 'unit_cost']
            missing = [col for col in required if col not in df.columns]
            if missing:
                raise ValidationError(f"Missing mandatory columns in CSV: {missing}")
                
            inserted = 0
            updated = 0
            
            # Process in transaction to ensure atomic batch execution
            with self.db.transaction() as cursor:
                for idx, row in df.iterrows():
                    sku = str(row['sku']).strip()
                    name = str(row['name']).strip()
                    
                    if not sku or not name:
                        raise ValidationError(f"Row {idx+1}: SKU and Name cannot be empty.")
                        
                    stock = int(row['current_stock'])
                    cost = float(row['unit_cost'])
                    reorder_point = int(row.get('reorder_point', 10))
                    
                    # Validate
                    if stock < 0 or cost < 0 or reorder_point < 0:
                        raise ValidationError(f"Row {idx+1}: Negative values are invalid for stock, cost, or reorder point.")
                        
                    # Check existing by SKU
                    cursor.execute("SELECT id, name FROM products WHERE sku = ?;", (sku,))
                    existing = cursor.fetchone()
                    
                    if existing:
                        # Update
                        cursor.execute("""
                            UPDATE products 
                            SET name=?, current_stock=?, unit_cost=?, reorder_point=?, updated_at=datetime('now')
                            WHERE id=?;
                        """, (name, stock, cost, reorder_point, existing['id']))
                        updated += 1
                    else:
                        # Check name duplicate among current db items
                        cursor.execute("SELECT id FROM products WHERE name = ?;", (name,))
                        if cursor.fetchone():
                            raise DuplicateProductError(f"Row {idx+1}: Product name '{name}' already exists in database with a different SKU.")
                            
                        # Insert
                        cursor.execute("""
                            INSERT INTO products (sku, name, category, current_stock, unit_cost, reorder_point, created_at, updated_at)
                            VALUES (?, ?, ?, ?, ?, ?, datetime('now'), datetime('now'));
                        """, (sku, name, str(row.get('category', '')), stock, cost, reorder_point))
                        inserted += 1
                        
            logger.info(f"Bulk CSV import complete: {inserted} inserted, {updated} updated.")
            return {"inserted": inserted, "updated": updated}
            
        except ValidationError:
            raise
        except Exception as e:
            logger.error(f"Bulk CSV import error: {e}", exc_info=True)
            raise ValidationError(f"Failed to process CSV file: {e}") from e

    # ─────────────────────────────────────────────
    # Inventory Tracking & Deliveries
    # ─────────────────────────────────────────────

    def record_delivery(
        self,
        sku: str,
        quantity: int,
        supplier_name: str = "",
        cost_per_unit: float = 0.0,
        reason: str = "Weekly Delivery",
        movement_date: Optional[str] = None,
        entered_by: Optional[int] = None
    ) -> int:
        """
        Records a product delivery ('IN') and automatically updates current stock.
        """
        product = self.db.get_product_by_sku(sku)
        if not product:
            raise ValidationError(f"Product with SKU '{sku}' does not exist.")
            
        if movement_date is None:
            movement_date = datetime.now().strftime("%Y-%m-%d")
            
        validate_stock_movement_data(product.id, "IN", quantity, movement_date)
        validate_non_negative(cost_per_unit, "cost_per_unit")
        
        movement = StockMovement(
            product_id=product.id,
            movement_type="IN",
            quantity=quantity,
            supplier_name=supplier_name,
            cost_per_unit=cost_per_unit or product.unit_cost,
            reason=reason,
            movement_date=movement_date,
            entered_by=entered_by
        )
        return self.db.add_stock_movement(movement)

    def submit_weekly_stock(
        self,
        sku: str,
        remaining_stock: int,
        entry_date: str,
        entered_by: Optional[int] = None
    ) -> Dict[str, Any]:
        """
        Computes weekly sales, previous stock, and deliveries.
        Saves calculated sales to daily_entries and updates current_stock.
        """
        validate_date_format(entry_date, "entry_date")
        validate_non_negative(remaining_stock, "remaining_stock")
        
        product = self.db.get_product_by_sku(sku)
        if not product:
            raise ValidationError(f"Product SKU='{sku}' does not exist.")
            
        try:
            # We wrap the entire calculation and update in a transaction
            with self.db.transaction() as cursor:
                # 1. Fetch current stock before updating (which has deliveries loaded)
                cursor.execute("SELECT current_stock, unit_cost FROM products WHERE id = ?;", (product.id,))
                prod_row = cursor.fetchone()
                current_stock_db = prod_row["current_stock"]
                
                # 2. Find date of last weekly entry
                cursor.execute("""
                    SELECT entry_date FROM daily_entries 
                    WHERE product_id = ? 
                    ORDER BY entry_date DESC LIMIT 1;
                """, (product.id,))
                last_entry = cursor.fetchone()
                last_date = last_entry["entry_date"] if last_entry else "1970-01-01"
                
                # 3. Sum all deliveries ('IN') since the last entry date
                cursor.execute("""
                    SELECT SUM(quantity) as deliv_sum FROM stock_movements
                    WHERE product_id = ? AND movement_type = 'IN' AND movement_date > ? AND movement_date <= ?;
                """, (product.id, last_date, entry_date))
                sum_row = cursor.fetchone()
                deliveries = sum_row["deliv_sum"] or 0
                
                # Previous stock at start of period is: Stock Now (before remaining stock update) - Deliveries
                previous_stock = max(0, current_stock_db - deliveries)
                
                # Compute Sales: Sales = Previous Stock + Deliveries - remaining_stock
                # Equivalent to current_stock_db - remaining_stock
                weekly_sales = current_stock_db - remaining_stock
                weekly_sales = max(0, weekly_sales)  # Clamp negatives to 0
                
                # 4. Insert calculated sales into daily_entries
                cursor.execute("""
                    INSERT INTO daily_entries (product_id, entry_date, units_sold, units_wasted, entered_by, created_at)
                    VALUES (?, ?, ?, 0, ?, datetime('now'));
                """, (product.id, entry_date, weekly_sales, entered_by))
                
                # 5. Update product stock level to remaining stock
                cursor.execute("""
                    UPDATE products 
                    SET current_stock = ?, updated_at = datetime('now')
                    WHERE id = ?;
                """, (remaining_stock, product.id))
                
                logger.info(
                    f"Weekly submission for SKU={sku}: Sales={weekly_sales}, "
                    f"PrevStock={previous_stock}, Deliveries={deliveries}, NewStock={remaining_stock}"
                )
                
                return {
                    "weekly_sales": weekly_sales,
                    "previous_stock": previous_stock,
                    "deliveries": deliveries,
                    "current_stock": remaining_stock
                }
                
        except Exception as e:
            logger.error(f"Weekly stock submission failed for SKU={sku}: {e}", exc_info=True)
            raise
