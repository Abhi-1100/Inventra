#include "python/PythonBridge.h"
#include "core/AppController.h"  // AppSettings
#include <QDir>
#include <QCoreApplication>
#include <QDebug>

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
        static py::scoped_interpreter guard{};

        py::module_ sys = py::module_::import("sys");

        // Add our python/ folder to sys.path
        sys.attr("path").attr("insert")(0, pythonModulePath.toStdString());

        // Also add the parent directory (build/) so 'backend' package is found
        QDir pythonDir(pythonModulePath);
        pythonDir.cdUp();  // go up from build/python/ to build/
        sys.attr("path").attr("insert")(0, pythonDir.absolutePath().toStdString());

        // Pre-import the bridge module to catch import errors early
        py::module_::import("inventra_bridge");

        m_initialized = true;
        qDebug() << "[PythonBridge] Initialized successfully. Python path:"
                 << pythonModulePath;
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
// Helper: deserialize one result dict → MLResult
// ─────────────────────────────────────────────

#ifdef KIRANA_PYTHON_ENABLED
static MLResult deserializeMLResult(py::object item) {
    MLResult r;

    r.productId     = item["product_id"].cast<int>();
    r.sku           = QString::fromStdString(item["sku"].cast<std::string>());
    r.confidence    = item["confidence"].cast<double>();
    r.forecastNext7 = item["forecast_7d"].cast<double>();
    r.forecastTrend = item["trend"].cast<double>();
    r.eoqQty        = item["eoq"].cast<int>();

    // Textual outputs
    if (py::isinstance<py::str>(item["explanation"]))
        r.explanation   = QString::fromStdString(item["explanation"].cast<std::string>());
    if (py::isinstance<py::str>(item["recommendation"]))
        r.recommendation= QString::fromStdString(item["recommendation"].cast<std::string>());

    // Demand label
    const std::string dl = item["demand_label"].cast<std::string>();
    if      (dl == "High"   || dl == "High Demand")   r.demandLabel = DemandLabel::High;
    else if (dl == "Medium" || dl == "Medium Demand") r.demandLabel = DemandLabel::Medium;
    else if (dl == "Low"    || dl == "Low Demand")    r.demandLabel = DemandLabel::Low;
    else                                               r.demandLabel = DemandLabel::Unknown;

    // Stock status
    const std::string ss = item["stock_status"].cast<std::string>();
    if      (ss == "Reorder"  || ss == "Critical" || ss == "Low") r.stockStatus = StockStatus::Reorder;
    else if (ss == "Overstock")                                    r.stockStatus = StockStatus::Overstock;
    else if (ss == "NoAction" || ss == "Healthy")                  r.stockStatus = StockStatus::NoAction;
    else                                                            r.stockStatus = StockStatus::NoAction;

    // Priority
    const std::string pr = item["priority"].cast<std::string>();
    if      (pr == "Critical")                            r.priority = Priority::Critical;
    else if (pr == "Reorder Soon" || pr == "ReorderSoon") r.priority = Priority::ReorderSoon;
    else                                                   r.priority = Priority::Safe;

    // Forecast points
    if (py::isinstance<py::list>(item["forecast_points"])) {
        auto pts = item["forecast_points"].cast<py::list>();
        for (auto pt_item_raw : pts) {
            try {
                auto pt_item = pt_item_raw.cast<py::dict>();
                ForecastPoint pt;
                pt.date  = QDate::fromString(
                    QString::fromStdString(pt_item["ds"].cast<std::string>()),
                    QStringLiteral("yyyy-MM-dd"));
                pt.value = pt_item["yhat"].cast<double>();
                pt.lower = pt_item["yhat_lower"].cast<double>();
                pt.upper = pt_item["yhat_upper"].cast<double>();
                r.forecast.append(pt);
            } catch (...) {
                // skip malformed forecast points
            }
        }
    }

    return r;
}
#endif // KIRANA_PYTHON_ENABLED

// ─────────────────────────────────────────────
// runPipeline()
// Main entry point: run all products.
// csvPath empty → use SQLite DB mode via inventra_bridge.
// csvPath set   → use CSV file via legacy python/ modules.
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
            // ── DB Mode: use inventra_bridge.predict_all_products() ──
            auto bridge = py::module_::import("inventra_bridge");

            auto py_results = bridge.attr("predict_all_products")(
                settings.dbPath.toStdString(),
                settings.leadTimeDays,
                settings.orderingCost,
                settings.holdingCostRate,
                settings.forecastHorizonDays
            );

            for (auto item_raw : py_results) {
                auto item = item_raw.cast<py::dict>();
                result.results.append(deserializeMLResult(item));
            }

        } else {
            // ── CSV Mode: use inventra_bridge.import_csv_and_predict() ──
            auto bridge = py::module_::import("inventra_bridge");

            auto py_results = bridge.attr("import_csv_and_predict")(
                csvPath.toStdString(),
                settings.dbPath.toStdString()
            );

            for (auto item_raw : py_results) {
                auto item = item_raw.cast<py::dict>();
                result.results.append(deserializeMLResult(item));
            }
            result.isImport = true; // New flag we need to add to PipelineRunResult
        }

        result.success   = true;
        result.totalRows = static_cast<int>(result.results.size());

    } catch (const py::error_already_set& e) {
        result.errorMsg = QString::fromStdString(e.what());
        qCritical("[PythonBridge] runPipeline error: %s", e.what());
    } catch (const std::exception& e) {
        result.errorMsg = QString::fromStdString(e.what());
        qCritical("[PythonBridge] runPipeline std error: %s", e.what());
    }
#else
    // Phase 1: return empty success so callers don't crash
    result.success = true;
    Q_UNUSED(csvPath)
    Q_UNUSED(settings)
#endif

    return result;
}

// ─────────────────────────────────────────────
// runSingleProduct()
// Predict one product by ID via inventra_bridge.predict_single_product().
// ─────────────────────────────────────────────

MLResult PythonBridge::runSingleProduct(int productId, const AppSettings& settings) {
    MLResult result;
    result.productId = productId;

#ifdef KIRANA_PYTHON_ENABLED
    if (!m_initialized) {
        result.explanation = "Python not initialised.";
        return result;
    }

    try {
        py::gil_scoped_acquire acquire;

        auto bridge = py::module_::import("inventra_bridge");
        auto py_result = bridge.attr("predict_single_product")(
            productId,
            settings.dbPath.toStdString(),
            settings.leadTimeDays,
            settings.orderingCost,
            settings.holdingCostRate,
            settings.forecastHorizonDays
        );

        if (py_result.is_none()) {
            result.explanation = QString("Product ID %1 not found in database.").arg(productId);
            return result;
        }

        result = deserializeMLResult(py_result.cast<py::dict>());

    } catch (const py::error_already_set& e) {
        result.explanation = QString("Python error: ") + QString::fromStdString(e.what());
        qCritical("[PythonBridge] runSingleProduct error: %s", e.what());
    } catch (const std::exception& e) {
        result.explanation = QString("Error: ") + QString::fromStdString(e.what());
    }
#else
    Q_UNUSED(settings)
#endif

    return result;
}

} // namespace Kirana
