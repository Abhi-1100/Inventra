#pragma once

#include <QWidget>
#include <QDateTime>

class QLabel;
class QPushButton;

namespace Kirana {

// ─────────────────────────────────────────────
// StatusBar — top 28px status strip
//
// Layout (left → right):
//   [APP TITLE + STORE]   [● PIPELINE: LIVE|IDLE]   [LAST RUN: HH:mm:ss] [RUN NOW]
// ─────────────────────────────────────────────

class StatusBar : public QWidget {
    Q_OBJECT

public:
    explicit StatusBar(QWidget* parent = nullptr);

    void setPipelineState(bool live, const QDateTime& lastRun);

signals:
    void runNowRequested();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QLabel*      m_pipelineLabel = nullptr;
    QLabel*      m_lastRunLabel  = nullptr;
    QPushButton* m_runBtn        = nullptr;

    void buildLayout();
};

} // namespace Kirana
