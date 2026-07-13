#include "python/PythonBridge.h"
#include "core/AppController.h"  // AppSettings
#include <QDir>
#include <QCoreApplication>

#ifdef KIRANA_PYTHON_ENABLED
// Solve Qt-Python 'slots' macro conflict
#  pragma push_macro("slots")
#  undef slots
// pybind11 embed header — initialises CPython interpreter
#  include <pybind11/embed.h>
#  include <pybind11/stl.h>
#  pragma pop_macro("slots")
namespace py = pybind11;
#endif

namespace Kirana {

// ─────────────────────────────────────────────
// Singleton accessor
// ─────────────────────────────────────────────

PythonBridge& PythonBridge::instance() {
    static PythonBridge bridge;
    return bridge;
}

// ─────────────────────────────────────────────
// initialize()
// ─────────────────────────────────────────────

bool PythonBridge::initialize(const QString& pythonModulePath) {
    if (m_initialized) return true;

#ifdef KIRANA_PYTHON_ENABLED
    try {
        // scoped_interpreter must be created on the main thread
        // and kept alive for the application lifetime.
        // Use a static guard so it persists.
        static py::scoped_interpreter guard{};

        // Add our python/ folder to sys.path
        py::module_ sys = py::module_::import("sys");
        sys.attr("path").attr("insert")(0, pythonModulePath.toStdString());

        // Pre-import heavy modules during startup
        py::module_::import("data_module");
        py::module_::import("model_module");
        py::module_::import("prediction_module");

        m_initialized = true;
    } catch (const py::error_already_set& e) {
        qCritical("PythonBridge::initialize failed: %s", e.what());
        return false;
    }
#else
    Q_UNUSED(pythonModulePath)
    // Phase 1: no-op
    m_initialized = true;
#endif

    return m_initialized;
}

// ─────────────────────────────────────────────
// shutdown()
// ─────────────────────────────────────────────

void PythonBridge::shutdown() {
    // In pybind11, scoped_interpreter handles cleanup automatically.
    m_initialized = false;
}

// ─────────────────────────────────────────────
// runPipeline()
// ─────────────────────────────────────────────

PipelineRunResult PythonBridge::runPipeline(
    const QString& csvPath,
    const AppSettings& settings)
{
    PipelineRunResult result;

#ifdef KIRANA_PYTHON_ENABLED
    if (!m_initialized) {
        result.errorMsg = "Python interpreter not initialised.";
        return result;
    }

    try {
        py::gil_scoped_acquire acquire; // acquire GIL on worker thread

        if (csvPath.isEmpty()) {
            // Run predictions directly on the SQLite database!
            auto db_mod   = py::module_::import("backend.database.db_manager");
            auto pred_svc_mod = py::module_::import("backend.services.prediction_service");

            // Instantiate DBManager and PredictionService
            auto db_manager = db_mod.attr("DBManager")(settings.dbPath.toStdString());
            auto service = pred_svc_mod.attr("PredictionService")(db_manager);

            // Run predictions
            auto py_results = service.attr("run_predictions")(
                settings.leadTimeDays,
                settings.orderingCost,
                settings.holdingCostRate,
                settings.forecastHorizonDays
            );

            // Deserialise python list of MLResult objects
            for (auto item_raw : py_results) {
                auto item = item_raw.cast<py::object>();
                MLResult r;
                r.productId      = item.attr("product_id").cast<int>();
                r.sku            = QString::fromStdString(item.attr("sku").cast<std::string>());
                r.confidence     = item.attr("confidence").cast<double>();
                r.forecastNext7  = item.attr("forecast_7d").cast<double>();
                r.forecastTrend  = item.attr("trend").cast<double>();
                r.eoqQty         = item.attr("eoq").cast<int>();

                const std::string dl = item.attr("demand_label").cast<std::string>();
                if      (dl == "High" || dl == "High Demand")   r.demandLabel = DemandLabel::High;
                else if (dl == "Medium" || dl == "Medium Demand") r.demandLabel = DemandLabel::Medium;
                else                                              r.demandLabel = DemandLabel::Low;

                const std::string ss = item.attr("stock_status").cast<std::string>();
                if      (ss == "Reorder")   r.stockStatus = StockStatus::Reorder;
                else if (ss == "Overstock") r.stockStatus = StockStatus::Overstock;
                else                        r.stockStatus = StockStatus::NoAction;

                const std::string pr = item.attr("priority").cast<std::string>();
                if      (pr == "Critical")    r.priority = Priority::Critical;
                else if (pr == "ReorderSoon" || pr == "Reorder Soon") r.priority = Priority::ReorderSoon;
                else                          r.priority = Priority::Safe;

                if (py::hasattr(item, "forecast_points")) {
                    auto pts = item.attr("forecast_points").cast<py::list>();
                    for (auto pt_item_raw : pts) {
                        auto pt_item = pt_item_raw.cast<py::dict>();
                        ForecastPoint pt;
                        pt.date = QDate::fromString(QString::fromStdString(pt_item["ds"].cast<std::string>()), QStringLiteral("yyyy-MM-dd"));
                        pt.value = pt_item["yhat"].cast<double>();
                        pt.lower = pt_item["yhat_lower"].cast<double>();
                        pt.upper = pt_item["yhat_upper"].cast<double>();
                        r.forecast.append(pt);
                    }
                }

                result.results.append(r);
            }
        } else {
            auto data_mod   = py::module_::import("data_module");
            auto model_mod  = py::module_::import("model_module");
            auto pred_mod   = py::module_::import("prediction_module");

            // 1. Load + clean CSV
            auto df = data_mod.attr("load_csv")(csvPath.toStdString());

            // 2. Run classification pipeline
            auto ml_results = model_mod.attr("run_pipeline")(df);

            // 3. Run Prophet forecasts + EOQ
            auto forecasts = pred_mod.attr("forecast_all")(
                df,
                ml_results,
                settings.forecastHorizonDays,
                settings.orderingCost,
                settings.holdingCostRate
            );

            // 4. Deserialise Python results into C++ MLResult structs
            for (auto item : forecasts) {
                MLResult r;
                r.productId      = item["product_id"].cast<int>();
                r.sku            = QString::fromStdString(item["sku"].cast<std::string>());
                r.confidence     = item["confidence"].cast<double>();
                r.forecastNext7  = item["forecast_7d"].cast<double>();
                r.forecastTrend  = item["trend"].cast<double>();
                r.eoqQty         = item["eoq"].cast<int>();

                const std::string dl = item["demand_label"].cast<std::string>();
                if      (dl == "High" || dl == "High Demand")   r.demandLabel = DemandLabel::High;
                else if (dl == "Medium" || dl == "Medium Demand") r.demandLabel = DemandLabel::Medium;
                else                                              r.demandLabel = DemandLabel::Low;

                const std::string ss = item["stock_status"].cast<std::string>();
                if      (ss == "Reorder")   r.stockStatus = StockStatus::Reorder;
                else if (ss == "Overstock") r.stockStatus = StockStatus::Overstock;
                else                        r.stockStatus = StockStatus::NoAction;

                const std::string pr = item["priority"].cast<std::string>();
                if      (pr == "Critical")    r.priority = Priority::Critical;
                else if (pr == "ReorderSoon" || pr == "Reorder Soon") r.priority = Priority::ReorderSoon;
                else                          r.priority = Priority::Safe;

                if (item.contains("forecast_points")) {
                    auto pts = item["forecast_points"].cast<py::list>();
                    for (auto pt_item_raw : pts) {
                        auto pt_item = pt_item_raw.cast<py::dict>();
                        ForecastPoint pt;
                        pt.date = QDate::fromString(QString::fromStdString(pt_item["ds"].cast<std::string>()), QStringLiteral("yyyy-MM-dd"));
                        pt.value = pt_item["yhat"].cast<double>();
                        pt.lower = pt_item["yhat_lower"].cast<double>();
                        pt.upper = pt_item["yhat_upper"].cast<double>();
                        r.forecast.append(pt);
                    }
                }

                result.results.append(r);
            }
        }

        result.success   = true;
        result.totalRows = static_cast<int>(result.results.size());

    } catch (const py::error_already_set& e) {
        result.errorMsg = QString::fromStdString(e.what());
    } catch (const std::exception& e) {
        result.errorMsg = QString::fromStdString(e.what());
    }
#else
    // Phase 1: return empty success so callers don't crash
    result.success = true;
    Q_UNUSED(csvPath)
    Q_UNUSED(settings)
#endif

    return result;
}

} // namespace Kirana
