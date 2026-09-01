# Data Mapping: FMCG Retail Sales to Inventra Training Schema

This document details the exact mapping and transformations applied to convert the `Indian FMCG Retail Sales Customer Inventory (2024).csv` dataset into the Inventra `inventra_training_data.csv` schema.

## Inventory Entity Definition

Before mapping individual fields, it's crucial to define what an "inventory entity" is in the source data.
Analysis showed that `Stock_On_Hand` varies independently across different cities for the exact same `Brand` + `Category` combination. Therefore, stock is tracked at the city level.
**Inventory Entity** = `City` + `Brand` + `Category`

## Field Mapping Table

| Inventra field | Source field | Transformation |
| :--- | :--- | :--- |
| **`product_id`** | `City`, `Brand`, `Category` | Concatenated as `{CITY}_{BRAND}_{CATEGORY}`, converted to uppercase with spaces replaced by underscores (e.g., `MUMBAI_NESTLE_GROCERY`). |
| **`product_name`** | `City`, `Brand`, `Category` | Formatted as a human-readable string: `{Brand} {Category} ({City})` (e.g., `Nestle Grocery (Mumbai)`). |
| **`category`** | `Category` | Preserved exactly as provided in the source dataset. |
| **`week_start_date`** | `Invoice_Date` | Dates were aggregated into Monday-based ISO weeks. The value represents the Monday (`YYYY-MM-DD`) of the week in which the transactions occurred. |
| **`weekly_sales`** | `Units` | The `SUM` of all `Units` sold for the specific inventory entity within that week. If a week had no transactions, this is set to `0`. |
| **`closing_stock`** | `Stock_On_Hand` | The `Stock_On_Hand` value from the chronologically **LAST** transaction for that entity in that week. If a week had no transactions, the value was **forward-filled** from the previous known week. |
| **`unit_price_inr`** | `Selling_Price`, `Revenue`, `Units`| Calculated as the **Revenue-weighted average** for the week: `SUM(Revenue) / SUM(Units)`. Forward-filled for weeks with no sales. |
| **`reorder_point`** | `Reorder_Level` | Calculated as the **MEDIAN** `Reorder_Level` across all transactions for that entity in that week (rounded to integer). Forward-filled for weeks with no sales. |
| **`lead_time`** | `Lead_Time_Days` | Calculated as the **MEDIAN** `Lead_Time_Days` across all transactions for that entity in that week (rounded to integer). Forward-filled for weeks with no sales. |
| **`order_cost`** | *(Not Available)* | **NOT AVAILABLE IN SOURCE**. Hardcoded to the Inventra pipeline default of `20.0` INR. |
| **`holding_cost`** | `Selling_Price` (derived) | **NOT AVAILABLE IN SOURCE**. Derived using the Inventra convention as 25% of the unit cost: `unit_price_inr * 0.25`. |

## Handling Missing Data & Gaps

* **Missing Weeks**: The source dataset is transaction-level. When converting to weekly time series, any weeks without transactions for an active product were filled:
  * `weekly_sales` = 0
  * `closing_stock`, `unit_price_inr`, `reorder_point`, `lead_time` = forward-filled from the last known observation (since inventory and parameters persist between sales events).
* **Missing Source Values**: `Customer_Age` and `Customer_Gender` had missing values in the source, but they are not used in the Inventra schema, so they were safely ignored.
