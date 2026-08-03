#pragma once

#include <QWidget>
#include <QDateTime>

class QLabel;
class QPushButton;
class QLineEdit;

namespace Kirana {

// ─────────────────────────────────────────────
// StatusBar — Top App Bar (56px)
//
// Layout (left → right):
//   [Search Input]  stretch  [Last sync]  [● LIVE|IDLE]  [Run Pipeline]  |  [🔔]  [👤]
// ─────────────────────────────────────────────

class StatusBar : public QWidget {
    Q_OBJECT

public:
    explicit StatusBar(QWidget* parent = nullptr);

    void setPipelineState(bool live, const QDateTime& lastRun);

signals:
    void searchRequested(const QString& query);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QLineEdit*   m_searchEdit     = nullptr;
    QLabel*      m_pipelineLabel  = nullptr;
    QLabel*      m_lastRunLabel   = nullptr;

    void buildLayout();
};

} // namespace Kirana
