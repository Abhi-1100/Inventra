#include "ui/StatusBar.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QPainter>
#include <QPaintEvent>
#include <QDateTime>

namespace Kirana {

// ─────────────────────────────────────────────
// StatusBar — Top App Bar
// Layout: [Search]  stretch  [Sync status]  [● pipe]  [Run Now]  [🔔]  [👤]
// Height: 56px, background: surface (#101419), border-bottom: #424754
// ─────────────────────────────────────────────

StatusBar::StatusBar(QWidget* parent)
    : QWidget(parent)
{
    setFixedHeight(56);
    setObjectName("TopAppBar");
    buildLayout();
    setPipelineState(false, QDateTime());
}

void StatusBar::buildLayout() {
    auto* lay = new QHBoxLayout(this);
    lay->setContentsMargins(20, 0, 20, 0);
    lay->setSpacing(0);

    // ── Search bar ────────────────────────────
    auto* searchWrap = new QWidget(this);
    searchWrap->setFixedWidth(300);
    searchWrap->setStyleSheet(
        "QWidget {"
        "  background-color: #0a0e13;"
        "  border: 1px solid #232a33;"
        "  border-radius: 6px;"
        "}"
        "QWidget:focus-within {"
        "  border-color: #afc6ff;"
        "}");

    auto* searchRow = new QHBoxLayout(searchWrap);
    searchRow->setContentsMargins(10, 0, 10, 0);
    searchRow->setSpacing(8);

    auto* searchIcon = new QLabel(QStringLiteral("⌕"), searchWrap);
    searchIcon->setStyleSheet(
        "font-size: 16px; color: #8c90a0; background: transparent; border: none;");
    searchIcon->setFixedWidth(18);

    m_searchEdit = new QLineEdit(searchWrap);
    m_searchEdit->setPlaceholderText(QStringLiteral("Search SKU, Product..."));
    m_searchEdit->setStyleSheet(
        "QLineEdit {"
        "  background: transparent;"
        "  border: none;"
        "  color: #e0e2ea;"
        "  font-family: 'Hanken Grotesk', sans-serif;"
        "  font-size: 14px;"
        "  padding: 0;"
        "}"
        "QLineEdit::placeholder { color: rgba(140,144,160,0.6); }");
    m_searchEdit->setFixedHeight(36);

    searchRow->addWidget(searchIcon);
    searchRow->addWidget(m_searchEdit);

    lay->addWidget(searchWrap);
    lay->addStretch();

    // ── Last sync label ──────────────────────
    m_lastRunLabel = new QLabel(this);
    m_lastRunLabel->setStyleSheet(
        "font-family: 'JetBrains Mono', monospace;"
        "font-size: 12px;"
        "color: #8c90a0;"
        "background: transparent;"
        "border: none;");
    lay->addWidget(m_lastRunLabel);
    lay->addSpacing(20);

    // ── Pipeline status ──────────────────────
    m_pipelineLabel = new QLabel(this);
    m_pipelineLabel->setStyleSheet(
        "font-family: 'JetBrains Mono', monospace;"
        "font-size: 11px;"
        "font-weight: 600;"
        "background: transparent;"
        "border: none;");
    lay->addWidget(m_pipelineLabel);
    lay->addSpacing(16);

    // ── Run Now button ────────────────────────
    m_runBtn = new QPushButton(QStringLiteral("Run Pipeline"), this);
    m_runBtn->setFixedHeight(32);
    m_runBtn->setCursor(Qt::PointingHandCursor);
    m_runBtn->setObjectName("PrimaryBtn");
    m_runBtn->setStyleSheet(
        "QPushButton {"
        "  background: qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #1f6feb,stop:1 #0050a7);"
        "  border: none;"
        "  border-top: 1px solid rgba(175,198,255,0.3);"
        "  color: #fffcff;"
        "  font-family: 'Hanken Grotesk', sans-serif;"
        "  font-size: 13px;"
        "  font-weight: 600;"
        "  border-radius: 6px;"
        "  padding: 0 16px;"
        "}"
        "QPushButton:hover {"
        "  background: qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #388bfd,stop:1 #1f6feb);"
        "}"
        "QPushButton:pressed { background: #0050a7; }");
    connect(m_runBtn, &QPushButton::clicked, this, &StatusBar::runNowRequested);
    lay->addWidget(m_runBtn);
    lay->addSpacing(16);

    // ── Separator ─────────────────────────────
    auto* sep = new QWidget(this);
    sep->setFixedSize(1, 20);
    sep->setStyleSheet("background: #424754; border: none;");
    lay->addWidget(sep);
    lay->addSpacing(16);

    // ── Notification bell ─────────────────────
    auto* notifBtn = new QPushButton(this);
    notifBtn->setFixedSize(32, 32);
    notifBtn->setCursor(Qt::PointingHandCursor);
    notifBtn->setText(QStringLiteral("🔔"));
    notifBtn->setStyleSheet(
        "QPushButton {"
        "  background: transparent; border: none;"
        "  font-size: 16px; color: #8c90a0; border-radius: 16px;"
        "}"
        "QPushButton:hover { background: #262a30; color: #afc6ff; }");
    lay->addWidget(notifBtn);
    lay->addSpacing(8);

    // ── Account button ────────────────────────
    auto* accountBtn = new QPushButton(this);
    accountBtn->setFixedSize(32, 32);
    accountBtn->setCursor(Qt::PointingHandCursor);
    accountBtn->setText(QStringLiteral("👤"));
    accountBtn->setStyleSheet(
        "QPushButton {"
        "  background: transparent; border: none;"
        "  font-size: 16px; color: #8c90a0; border-radius: 16px;"
        "}"
        "QPushButton:hover { background: #262a30; color: #afc6ff; }");
    lay->addWidget(accountBtn);
}

// ─────────────────────────────────────────────
// setPipelineState
// ─────────────────────────────────────────────

void StatusBar::setPipelineState(bool live, const QDateTime& lastRun) {
    if (live) {
        m_pipelineLabel->setText(QStringLiteral("● LIVE"));
        m_pipelineLabel->setStyleSheet(
            "font-family:'JetBrains Mono',monospace;"
            "font-size:11px;font-weight:700;"
            "color:#afc6ff;background:transparent;border:none;");
    } else {
        m_pipelineLabel->setText(QStringLiteral("○ IDLE"));
        m_pipelineLabel->setStyleSheet(
            "font-family:'JetBrains Mono',monospace;"
            "font-size:11px;font-weight:600;"
            "color:#8c90a0;background:transparent;border:none;");
    }

    if (lastRun.isValid()) {
        m_lastRunLabel->setText(
            QStringLiteral("Last sync: ") + lastRun.toString(QStringLiteral("HH:mm")));
    } else {
        m_lastRunLabel->setText(QStringLiteral("Last sync: —"));
    }
}

// ─────────────────────────────────────────────
// paintEvent — draw background + bottom border
// ─────────────────────────────────────────────

void StatusBar::paintEvent(QPaintEvent* event) {
    QPainter p(this);
    p.fillRect(rect(), QColor("#101419"));

    // Bottom separator line matching outline-variant
    p.setPen(QPen(QColor("#424754"), 1));
    p.drawLine(0, height() - 1, width(), height() - 1);

    QWidget::paintEvent(event);
}

} // namespace Kirana
