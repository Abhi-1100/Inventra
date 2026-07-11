#include "ui/StatusBar.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QPainter>
#include <QPaintEvent>

namespace Kirana {

StatusBar::StatusBar(QWidget* parent)
    : QWidget(parent)
{
    setFixedHeight(30);
    setObjectName("StatusBar");
    buildLayout();
    setPipelineState(false, QDateTime());
}

// ─────────────────────────────────────────────
// Layout
// ─────────────────────────────────────────────

void StatusBar::buildLayout() {
    auto* lay = new QHBoxLayout(this);
    lay->setContentsMargins(12, 0, 12, 0);
    lay->setSpacing(0);

    // ── App / store label ────────────────────
    auto* appLabel = new QLabel("KIRANA TERMINAL  |  Main Branch", this);
    appLabel->setStyleSheet(
        "font-family: 'Consolas', 'JetBrains Mono', monospace;"
        "font-size: 10px;"
        "font-weight: 600;"
        "letter-spacing: 1.2px;"
        "color: #8b949e;"
        "background: transparent;"
    );

    // ── Pipeline status pill ─────────────────
    m_pipelineLabel = new QLabel(this);
    m_pipelineLabel->setStyleSheet(
        "font-family: 'Consolas', 'JetBrains Mono', monospace;"
        "font-size: 10px;"
        "font-weight: 600;"
        "letter-spacing: 1px;"
        "background: transparent;"
    );
    m_pipelineLabel->setAlignment(Qt::AlignCenter);

    // ── Last run ─────────────────────────────
    m_lastRunLabel = new QLabel(this);
    m_lastRunLabel->setStyleSheet(
        "font-family: 'Consolas', 'JetBrains Mono', monospace;"
        "font-size: 10px;"
        "color: #484f58;"
        "background: transparent;"
    );

    // ── Run Now button ────────────────────────
    m_runBtn = new QPushButton("  ▶  RUN NOW", this);
    m_runBtn->setFixedHeight(20);
    m_runBtn->setCursor(Qt::PointingHandCursor);
    m_runBtn->setStyleSheet(
        "QPushButton {"
        "  font-family: 'Consolas', monospace;"
        "  font-size: 9px;"
        "  font-weight: 700;"
        "  letter-spacing: 1px;"
        "  color: #58a6ff;"
        "  background: rgba(88,166,255,0.10);"
        "  border: 1px solid rgba(88,166,255,0.30);"
        "  border-radius: 3px;"
        "  padding: 0 8px;"
        "}"
        "QPushButton:hover {"
        "  background: rgba(88,166,255,0.20);"
        "  border-color: #58a6ff;"
        "}"
        "QPushButton:pressed {"
        "  background: rgba(88,166,255,0.30);"
        "}"
    );

    connect(m_runBtn, &QPushButton::clicked, this, &StatusBar::runNowRequested);

    // Separator helper
    auto sep = [this]() -> QLabel* {
        auto* s = new QLabel("  ·  ", this);
        s->setStyleSheet("color: #30363d; background: transparent;");
        return s;
    };

    lay->addWidget(appLabel);
    lay->addStretch();
    lay->addWidget(m_pipelineLabel);
    lay->addWidget(sep());
    lay->addWidget(m_lastRunLabel);
    lay->addSpacing(12);
    lay->addWidget(m_runBtn);
}

// ─────────────────────────────────────────────
// setPipelineState
// ─────────────────────────────────────────────

void StatusBar::setPipelineState(bool live, const QDateTime& lastRun) {
    if (live) {
        m_pipelineLabel->setText("◉  PIPELINE: LIVE");
        m_pipelineLabel->setStyleSheet(
            m_pipelineLabel->styleSheet()
            .replace("color: #", "color_unused: #")   // reset any old colour
        );
        m_pipelineLabel->setStyleSheet(
            "font-family:'Consolas',monospace;font-size:10px;font-weight:700;"
            "letter-spacing:1px;color:#3fb950;background:transparent;"
        );
    } else {
        m_pipelineLabel->setText("○  PIPELINE: IDLE");
        m_pipelineLabel->setStyleSheet(
            "font-family:'Consolas',monospace;font-size:10px;font-weight:600;"
            "letter-spacing:1px;color:#8b949e;background:transparent;"
        );
    }

    if (lastRun.isValid()) {
        m_lastRunLabel->setText(
            "LAST RUN: " + lastRun.toString("HH:mm:ss"));
    } else {
        m_lastRunLabel->setText("LAST RUN: —");
    }
}

// ─────────────────────────────────────────────
// paintEvent — draw bottom separator
// ─────────────────────────────────────────────

void StatusBar::paintEvent(QPaintEvent* event) {
    QPainter p(this);
    p.fillRect(rect(), QColor("#0d1117"));

    // Bottom border line
    p.setPen(QColor("#21262d"));
    p.drawLine(0, height() - 1, width(), height() - 1);

    QWidget::paintEvent(event);
}

} // namespace Kirana
