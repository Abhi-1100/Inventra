#pragma once

#include <QString>
#include "core/ProductData.h"
#include "python/PipelineResult.h"

namespace Kirana {

struct AppSettings;

// ─────────────────────────────────────────────
// PythonBridge — singleton
//
// Manages the embedded Python interpreter lifetime
// (must outlive all pybind11 calls) and provides
// a clean C++ API over the ML pipeline modules.
// ─────────────────────────────────────────────

class PythonBridge {
public:
    static PythonBridge& instance();

    // Must be called once on the main thread before any worker thread uses Python.
    // Adds the /python subfolder to sys.path and imports the three ML modules.
    bool initialize(const QString& pythonModulePath);
    void shutdown();

    bool isInitialized() const { return m_initialized; }

    // ── API called from PipelineWorker (background thread) ──

    // Run full pipeline against SQLite DB (csvPath empty) or CSV file.
    PipelineRunResult runPipeline(const QString& csvPath, const AppSettings& settings);

    // Run pipeline for a single product by ID (for detail panel "Run Prediction" button).
    MLResult runSingleProduct(int productId, const AppSettings& settings);

private:
    PythonBridge()  = default;
    ~PythonBridge() = default;

    bool m_initialized = false;

    // Prevent copy/move
    PythonBridge(const PythonBridge&) = delete;
    PythonBridge& operator=(const PythonBridge&) = delete;
};

} // namespace Kirana
