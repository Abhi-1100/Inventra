"""
Analytics Service for the AI Inventory Management Backend.
Generates business intelligence metrics, sales rankings, stock turnover ratios,
historical movement reports, and category performance summaries.
"""

import json
from datetime import datetime, timedelta
from typing import List, Dict, Any, Optional

from backend.database.db_manager import DBManager
from backend.utils.logger import get_logger

logger = get_logger("analytics_service")

class AnalyticsService:
    """
    Computes business reports and dashboard indicators.
    """
    def __init__(self, db_manager: DBManager):
        self.db = db_manager

    def get_top_selling_products(self, limit: int = 10) -> List[Dict[str, Any]]:
        """
        Retrieves products ranked by total volume sold.
        """
        query = """
        SELECT p.id, p.sku, p.name, p.category, SUM(de.units_sold) as total_sold, p.unit_cost
        FROM daily_entries de
        JOIN products p ON de.product_id = p.id
        GROUP BY p.id
        ORDER BY total_sold DESC
        LIMIT ?;
        """
        try:
            with self.db.connection() as conn:
                cursor = conn.cursor()
                cursor.execute(query, (limit,))
                return [dict(row) for row in cursor.fetchall()]
        except Exception as e:
            logger.error(f"Error fetching top selling products: {e}")
            return []

    def get_fastest_moving_inventory(self, limit: int = 10, days: int = 30) -> List[Dict[str, Any]]:
        """
        Retrieves products with the highest daily sales average over the specified window.
        """
        date_cutoff = (datetime.now() - timedelta(days=days)).strftime("%Y-%m-%d")
        query = """
        SELECT p.id, p.sku, p.name, p.category, SUM(de.units_sold) as total_sold, 
               AVG(de.units_sold) as avg_daily_sales, p.current_stock
        FROM products p
        LEFT JOIN daily_entries de ON de.product_id = p.id AND de.entry_date >= ?
        GROUP BY p.id
        ORDER BY avg_daily_sales DESC
        LIMIT ?;
        """
        try:
            with self.db.connection() as conn:
                cursor = conn.cursor()
                cursor.execute(query, (date_cutoff, limit))
                return [dict(row) for row in cursor.fetchall()]
        except Exception as e:
            logger.error(f"Error fetching fastest moving inventory: {e}")
            return []

    def get_slow_moving_inventory(self, limit: int = 10, days: int = 30) -> List[Dict[str, Any]]:
        """
        Retrieves items with low turnover rates (high current stock, low sales volume).
        """
        date_cutoff = (datetime.now() - timedelta(days=days)).strftime("%Y-%m-%d")
        query = """
        SELECT p.id, p.sku, p.name, p.category, p.current_stock, 
               COALESCE(SUM(de.units_sold), 0) as total_sold, p.unit_cost
        FROM products p
        LEFT JOIN daily_entries de ON de.product_id = p.id AND de.entry_date >= ?
        GROUP BY p.id
        ORDER BY total_sold ASC, p.current_stock DESC
        LIMIT ?;
        """
        try:
            with self.db.connection() as conn:
                cursor = conn.cursor()
                cursor.execute(query, (date_cutoff, limit))
                return [dict(row) for row in cursor.fetchall()]
        except Exception as e:
            logger.error(f"Error fetching slow moving inventory: {e}")
            return []

    def get_inventory_turnover(self, days: int = 365) -> Dict[str, float]:
        """
        Calculates the inventory turnover ratio: COGS / Average Inventory.
        COGS (Cost of Goods Sold) = Sum of units_sold * unit_cost over time period.
        Average Inventory = Sum of current_stock * unit_cost across products.
        """
        date_cutoff = (datetime.now() - timedelta(days=days)).strftime("%Y-%m-%d")
        
        cogs_query = """
        SELECT SUM(de.units_sold * p.unit_cost) as total_cogs
        FROM daily_entries de
        JOIN products p ON de.product_id = p.id
        WHERE de.entry_date >= ?;
        """
        
        inventory_value_query = """
        SELECT SUM(current_stock * unit_cost) as total_value, COUNT(id) as total_items
        FROM products;
        """
        
        try:
            with self.db.connection() as conn:
                cursor = conn.cursor()
                
                # 1. COGS
                cursor.execute(cogs_query, (date_cutoff,))
                cogs_row = cursor.fetchone()
                cogs = cogs_row["total_cogs"] or 0.0
                
                # 2. Average Inventory Value
                cursor.execute(inventory_value_query)
                inv_row = cursor.fetchone()
                avg_inv = inv_row["total_value"] or 0.0
                
                # Turn over ratio
                ratio = 0.0
                if avg_inv > 0:
                    ratio = cogs / avg_inv
                    
                return {
                    "cogs": float(cogs),
                    "average_inventory_value": float(avg_inv),
                    "inventory_turnover_ratio": float(round(ratio, 2))
                }
        except Exception as e:
            logger.error(f"Error computing inventory turnover: {e}")
            return {"cogs": 0.0, "average_inventory_value": 0.0, "inventory_turnover_ratio": 0.0}

    def get_products_below_reorder_point(self) -> List[Dict[str, Any]]:
        """
        Retrieves products whose current stock level is at or below their reorder point.
        """
        query = """
        SELECT id, sku, name, category, current_stock, reorder_point, unit_cost
        FROM products
        WHERE current_stock <= reorder_point
        ORDER BY current_stock ASC;
        """
        try:
            with self.db.connection() as conn:
                cursor = conn.cursor()
                cursor.execute(query)
                return [dict(row) for row in cursor.fetchall()]
        except Exception as e:
            logger.error(f"Error fetching products below reorder point: {e}")
            return []

    def get_delivery_history(self, product_id: Optional[int] = None, limit: int = 50) -> List[Dict[str, Any]]:
        """
        Retrieves history of product deliveries ('IN' stock movements).
        """
        if product_id:
            query = """
            SELECT sm.*, p.name as product_name, p.sku as product_sku
            FROM stock_movements sm
            JOIN products p ON sm.product_id = p.id
            WHERE sm.movement_type = 'IN' AND sm.product_id = ?
            ORDER BY sm.movement_date DESC, sm.created_at DESC
            LIMIT ?;
            """
            params = (product_id, limit)
        else:
            query = """
            SELECT sm.*, p.name as product_name, p.sku as product_sku
            FROM stock_movements sm
            JOIN products p ON sm.product_id = p.id
            WHERE sm.movement_type = 'IN'
            ORDER BY sm.movement_date DESC, sm.created_at DESC
            LIMIT ?;
            """
            params = (limit,)
            
        try:
            with self.db.connection() as conn:
                cursor = conn.cursor()
                cursor.execute(query, params)
                return [dict(row) for row in cursor.fetchall()]
        except Exception as e:
            logger.error(f"Error fetching delivery history: {e}")
            return []

    def get_historical_sales(self, product_id: Optional[int] = None, days: int = 30) -> List[Dict[str, Any]]:
        """
        Retrieves historical sales volume, grouped by date.
        """
        date_cutoff = (datetime.now() - timedelta(days=days)).strftime("%Y-%m-%d")
        if product_id:
            query = """
            SELECT entry_date, SUM(units_sold) as total_sold, SUM(units_wasted) as total_wasted
            FROM daily_entries
            WHERE product_id = ? AND entry_date >= ?
            GROUP BY entry_date
            ORDER BY entry_date ASC;
            """
            params = (product_id, date_cutoff)
        else:
            query = """
            SELECT entry_date, SUM(units_sold) as total_sold, SUM(units_wasted) as total_wasted
            FROM daily_entries
            WHERE entry_date >= ?
            GROUP BY entry_date
            ORDER BY entry_date ASC;
            """
            params = (date_cutoff,)
            
        try:
            with self.db.connection() as conn:
                cursor = conn.cursor()
                cursor.execute(query, params)
                return [dict(row) for row in cursor.fetchall()]
        except Exception as e:
            logger.error(f"Error fetching historical sales: {e}")
            return []

    def get_forecast_trends(self) -> List[Dict[str, Any]]:
        """
        Extracts forecast trends from the latest ML pipeline run.
        """
        try:
            results = self.db.get_latest_pipeline_results(limit=1)
            if not results or not results[0]["results_json"]:
                return []
                
            parsed_results = json.loads(results[0]["results_json"])
            trends = []
            
            for item in parsed_results:
                trends.append({
                    "product_id": item.get("product_id"),
                    "sku": item.get("sku"),
                    "forecast_7d": item.get("forecast_7d"),
                    "trend": item.get("trend"),
                    "priority": item.get("priority"),
                    "stock_status": item.get("stock_status")
                })
            return trends
        except Exception as e:
            logger.error(f"Failed to fetch forecast trends: {e}")
            return []

    def get_prediction_history(self, limit: int = 50) -> List[Dict[str, Any]]:
        """
        Retrieves past pipeline runs.
        """
        return self.db.get_latest_pipeline_results(limit=limit)

    def get_category_summaries(self) -> List[Dict[str, Any]]:
        """
        Summarizes inventory metrics grouped by category.
        """
        query = """
        SELECT category, COUNT(id) as product_count, SUM(current_stock) as total_stock,
               SUM(current_stock * unit_cost) as total_value, AVG(unit_cost) as avg_unit_cost
        FROM products
        GROUP BY category
        ORDER BY total_value DESC;
        """
        try:
            with self.db.connection() as conn:
                cursor = conn.cursor()
                cursor.execute(query)
                return [dict(row) for row in cursor.fetchall()]
        except Exception as e:
            logger.error(f"Error compiling category summaries: {e}")
            return []

    def get_monthly_summaries(self, limit_months: int = 12) -> List[Dict[str, Any]]:
        """
        Summarizes total sales and waste grouped by month (YYYY-MM).
        """
        query = """
        SELECT strftime('%Y-%m', entry_date) as month, 
               SUM(units_sold) as total_sold, 
               SUM(units_wasted) as total_wasted
        FROM daily_entries
        GROUP BY month
        ORDER BY month DESC
        LIMIT ?;
        """
        try:
            with self.db.connection() as conn:
                cursor = conn.cursor()
                cursor.execute(query, (limit_months,))
                # Return in chronological order
                results = [dict(row) for row in cursor.fetchall()]
                results.reverse()
                return results
        except Exception as e:
            logger.error(f"Error compiling monthly summaries: {e}")
            return []
