#include "python/PythonBridge.h"
#include "core/AppController.h"  // AppSettings
#include <QDir>
#include <QCoreApplication>

#ifdef KIRANA_PYTHON_ENABLED
// pybind11 embed header — initialises CPython interpreter
#  include <pybind11/embed.h>
#  include <pybind11/stl.h>
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
            r.confidence     = item["confidence"].cast<double>();
            r.forecastNext7  = item["forecast_7d"].cast<double>();
            r.forecastTrend  = item["trend"].cast<double>();
            r.eoqQty         = item["eoq"].cast<int>();

            const std::string dl = item["demand_label"].cast<std::string>();
            if      (dl == "High")   r.demandLabel = DemandLabel::High;
            else if (dl == "Medium") r.demandLabel = DemandLabel::Medium;
            else                     r.demandLabel = DemandLabel::Low;

            const std::string ss = item["stock_status"].cast<std::string>();
            if      (ss == "Reorder")   r.stockStatus = StockStatus::Reorder;
            else if (ss == "Overstock") r.stockStatus = StockStatus::Overstock;
            else                        r.stockStatus = StockStatus::NoAction;

            const std::string pr = item["priority"].cast<std::string>();
            if      (pr == "Critical")    r.priority = Priority::Critical;
            else if (pr == "ReorderSoon") r.priority = Priority::ReorderSoon;
            else                          r.priority = Priority::Safe;

            result.results.append(r);
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
