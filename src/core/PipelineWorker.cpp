#include "core/PipelineWorker.h"
#include "core/AppController.h"   // AppSettings

#ifdef KIRANA_PYTHON_ENABLED
#  include "python/PythonBridge.h"
#endif

#include <QElapsedTimer>
#include <QThread>

namespace Kirana {

PipelineWorker::PipelineWorker(QObject* parent)
    : QThread(parent)
{}

PipelineWorker::~PipelineWorker() {
    requestStop();
    wait(5000);
}

void PipelineWorker::setSettings(const AppSettings& s) {
    m_settings = s;
}

void PipelineWorker::requestStop() {
    m_abort = true;
}

// ─────────────────────────────────────────────
// run()  — executes on the worker thread
// ─────────────────────────────────────────────

void PipelineWorker::run() {
    emit pipelineStarted();
    m_abort = false;

    PipelineRunResult result;

#ifdef KIRANA_PYTHON_ENABLED
    // ── Phase 2: real Python ML pipeline ──────
    try {
        result = PythonBridge::instance().runPipeline(m_csvPath, m_settings);
        emit pipelineFinished(result);
    } catch (const std::exception& e) {
        emit pipelineError(QString::fromStdString(e.what()));
    }
#else
    // ── Phase 1: simulate pipeline progress ───
    result = runSimulated();
    if (!m_abort)
        emit pipelineFinished(result);
#endif
}

// ─────────────────────────────────────────────
// Simulated pipeline (Phase 1 placeholder)
// ─────────────────────────────────────────────

PipelineRunResult PipelineWorker::runSimulated() {
    const QStringList stages = {
        "Loading CSV…",
        "Cleaning data…",
        "K-Means clustering…",
        "RFE feature selection…",
        "SVM + SMOTE training…",
        "Cross-validating…",
        "Prophet forecasting…",
        "EOQ calculation…",
        "Finalising results…"
    };

    QElapsedTimer timer;
    timer.start();

    for (int i = 0; i < stages.size(); ++i) {
        if (m_abort) break;
        const int pct = static_cast<int>((i + 1) * 100 / stages.size());
        emit progressUpdate(pct, stages[i]);
        QThread::msleep(350);   // simulate work
    }

    PipelineRunResult res;
    res.success    = !m_abort;
    res.totalRows  = 25;
    res.durationMs = timer.elapsed();
    if (m_abort) res.errorMsg = "Aborted by user.";
    return res;
}

} // namespace Kirana
