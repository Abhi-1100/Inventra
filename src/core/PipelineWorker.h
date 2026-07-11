#pragma once

#include <QThread>
#include <QDateTime>
#include "core/ProductData.h"
#include "python/PipelineResult.h"

#include "core/AppController.h"

namespace Kirana {

// PipelineWorker
//
// Runs the ML pipeline on a background thread so
// the Qt GUI is never blocked.  In Phase 1 it
// emits a simulated progress sequence.  In Phase 2
// it delegates to PythonBridge::runPipeline().
// ─────────────────────────────────────────────

class PipelineWorker : public QThread {
    Q_OBJECT

public:
    explicit PipelineWorker(QObject* parent = nullptr);
    ~PipelineWorker() override;

    // Call before start()
    void setCsvPath(const QString& path) { m_csvPath = path; }
    void setSettings(const AppSettings& s);

    // Request graceful stop (sets m_abort flag)
    void requestStop();

signals:
    // Emitted during run (always on the worker thread — use queued connection)
    void pipelineStarted();
    void progressUpdate(int pct, const QString& stage);
    void pipelineFinished(const Kirana::PipelineRunResult& result);
    void pipelineError(const QString& message);

protected:
    void run() override;

private:
    QString     m_csvPath;
    AppSettings m_settings;
    bool        m_abort = false;

    PipelineRunResult runSimulated();
};

} // namespace Kirana
