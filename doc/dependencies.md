# Inventra Dependencies

This document details the dependencies and requirements for the **Inventra** application.

## Core Application
- **C++20**: The core application logic is built using modern C++20 features.
- **Qt6 (Widgets)**: Used for the graphical user interface. The UI relies on `QWidget` and `QTableView` with custom `QAbstractTableModel` for data grids. No QML is used.
- **SQLite**: Used for local persistence. The schema includes tables for:
  - `users` (id, name, role, pin_hash)
  - `shop_profile`
  - `products`
  - `daily_entries`
  - `stock_movements`
  - `session_settings`
  - `pipeline_results`

## ML Pipeline (Phase 2)
The application includes an embedded Python ML pipeline using the following dependencies:
- **Python 3**: Embedded interpreter.
- **pybind11**: Used for interoperability between C++ and Python.
- **pandas**: For data manipulation and analysis.
- **scikit-learn**: For machine learning tasks (e.g. classification).
- **imbalanced-learn**: For handling imbalanced datasets in classification.
- **prophet**: For time series forecasting of inventory demand.

## Build System
- **CMake**: Used to configure and generate the build system. Requires version 3.22+.

## Assets
- The application uses a custom logo mark and SVGs for iconography, compiled into a Qt Resource file (`inventra.qrc`).
