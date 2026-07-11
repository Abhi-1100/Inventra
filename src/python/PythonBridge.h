#pragma once

#include <QString>
#include "core/ProductData.h"
#include "python/PipelineResult.h"

// PythonBridge is only compiled when KIRANA_PYTHON_ENABLED is defined.
// In Phase 1, this file still exists but all methods are no-ops.

namespace Kirana {

struct AppSettings;

// ─────────────────────────────────────────────
// PythonBridge — singleton
//
// Manages the embedded Python interpreter lifetime
// (must outlive all pybind11 calls) and provides
// a clean C++ API over the three Python modules.
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
    PipelineRunResult runPipeline(const QString& csvPath, const AppSettings& settings);

private:
    PythonBridge()  = default;
    ~PythonBridge() = default;

    bool m_initialized = false;

    // Prevent copy/move
    PythonBridge(const PythonBridge&) = delete;
    PythonBridge& operator=(const PythonBridge&) = delete;
};

} // namespace Kirana
