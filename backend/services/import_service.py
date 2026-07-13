"""
Import Service for the AI Inventory Management Backend.
Handles ingesting raw CSV historical data into the SQLite database.
"""

import pandas as pd
from datetime import datetime, timedelta
from typing import Dict, Any

from backend.database.db_manager import DBManager
from backend.models.entities import Product, DailyEntry
from backend.utils.logger import get_logger

logger = get_logger("import_service")

class ImportService:
    def __init__(self, db_manager: DBManager):
        self.db = db_manager

    def import_csv(self, csv_path: str) -> bool:
        """
        Loads raw inventory history CSV, creates products, and generates 
        daily historical entries in the database.
        """
        logger.info(f"Starting CSV import from {csv_path}")
        try:
            df = pd.read_csv(csv_path)
            
            # Standardize column headers
            df.columns = [c.strip().lower().replace(" ", "_") for c in df.columns]
            
            # Map custom database columns from the dataset to standard names
            custom_mapping = {
                'product_id': 'sku',
                'product_name': 'name',
                'closing_stock': 'current_stock',
                'unit_price_inr': 'unit_cost',
                'weekly_sales': 'sales_volume',
                'week_start_date': 'date'
            }
            for src, dst in custom_mapping.items():
                if src in df.columns and dst not in df.columns:
                    df[dst] = df[src]
            
            # Fill missing values and enforce correct types
            df['current_stock'] = pd.to_numeric(df.get('current_stock', 0), errors='coerce').fillna(0).astype(int)
            df['unit_cost'] = pd.to_numeric(df.get('unit_cost', 0.0), errors='coerce').fillna(0.0)
            df['sales_volume'] = pd.to_numeric(df.get('sales_volume', 0), errors='coerce').fillna(0).astype(int)
            
            if 'date' in df.columns:
                df['date'] = pd.to_datetime(df['date'], errors='coerce')
            
            # Identify unique products
            products_added = 0
            entries_added = 0
            
            for sku, group in df.groupby('sku'):
                # Get the most recent data for the product info
                latest = group.sort_values(by='date', ascending=False).iloc[0] if 'date' in group.columns else group.iloc[0]
                
                # Check if product exists, if not create it
                existing_product = self.db.get_product_by_sku(str(sku))
                if existing_product:
                    product_id = existing_product.id
                else:
                    # Create new product
                    new_product = Product(
                        sku=str(sku),
                        name=str(latest.get('name', f"Product {sku}")),
                        category="Imported",
                        unit_cost=float(latest.get('unit_cost', 0.0)),
                        current_stock=int(latest.get('current_stock', 0)),
                        reorder_point=0
                    )
                    product_id = self.db.add_product(new_product)
                    products_added += 1
                
                # Add historical entries
                if 'date' in group.columns and 'sales_volume' in group.columns:
                    for _, row in group.iterrows():
                        if pd.notna(row['date']):
                            # We distribute weekly sales across 7 days roughly, or just treat it as a daily entry
                            entry = DailyEntry(
                                product_id=product_id,
                                entry_date=row['date'].strftime("%Y-%m-%d"),
                                units_sold=int(row['sales_volume'] / 7) if int(row['sales_volume']) > 7 else int(row['sales_volume']),
                                units_wasted=0
                            )
                            self.db.add_daily_entry(entry)
                            entries_added += 1
                else:
                    # Fake 30 days of history if no dates provided, based on sales_volume
                    daily_vol = int(latest.get('sales_volume', 30) / 30)
                    today = datetime.now()
                    for d in range(30):
                        entry_date = today - timedelta(days=(30 - d))
                        entry = DailyEntry(
                            product_id=product_id,
                            entry_date=entry_date.strftime("%Y-%m-%d"),
                            units_sold=daily_vol,
                            units_wasted=0
                        )
                        self.db.add_daily_entry(entry)
                        entries_added += 1

            logger.info(f"CSV Import complete. Added {products_added} products and {entries_added} daily entries.")
            return True
            
        except Exception as e:
            logger.error(f"Failed to import CSV: {e}")
            raise
