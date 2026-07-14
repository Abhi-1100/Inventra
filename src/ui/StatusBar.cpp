#include "ui/StatusBar.h"
#include "core/ThemeManager.h"

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
        "  background-color: #050505;"
        "  border: 1px solid #222222;"
        "  border-radius: 3px;"
        "}"
        "QWidget:focus-within {"
        "  border-color: #d97706;"
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
        "  color: #e5e5e5;"
        "  font-family: 'SF Mono', 'Menlo', 'Cascadia Mono', 'Consolas', monospace;"
        "  font-size: 13px;"
        "  padding: 0;"
        "}"
        "QLineEdit::placeholder { color: rgba(128,128,128,0.6); }");
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
    connect(m_runBtn, &QPushButton::clicked, this, &StatusBar::runNowRequested);
    lay->addWidget(m_runBtn);
    lay->addSpacing(16);

    // ── Separator ─────────────────────────────
    auto* sep = new QWidget(this);
    sep->setFixedSize(1, 20);
    sep->setStyleSheet("background: #222222; border: none;");
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
        "  font-size: 16px; color: #808080; border-radius: 16px;"
        "}"
        "QPushButton:hover { background: #161616; color: #d97706; }");
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
        "  font-size: 16px; color: #808080; border-radius: 16px;"
        "}"
        "QPushButton:hover { background: #161616; color: #d97706; }");
    lay->addWidget(accountBtn);
}

// ─────────────────────────────────────────────
// setPipelineState
// ─────────────────────────────────────────────

void StatusBar::setPipelineState(bool live, const QDateTime& lastRun) {
    const auto& tokens = ThemeManager::instance().tokens();
    if (live) {
        m_pipelineLabel->setText(QStringLiteral("● LIVE"));
        m_pipelineLabel->setStyleSheet(
            QString("font-family:'SF Mono','Menlo','Cascadia Mono','Consolas',monospace;"
                    "font-size:11px;font-weight:bold;"
                    "color:%1;background:transparent;border:none;").arg(tokens.Success.name()));
    } else {
        m_pipelineLabel->setText(QStringLiteral("○ IDLE"));
        m_pipelineLabel->setStyleSheet(
            QString("font-family:'SF Mono','Menlo','Cascadia Mono','Consolas',monospace;"
                    "font-size:11px;font-weight:bold;"
                    "color:%1;background:transparent;border:none;").arg(tokens.TextSecondary.name()));
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
    const auto& tokens = ThemeManager::instance().tokens();
    p.fillRect(rect(), tokens.BgPrimary);

    // Bottom separator line
    p.setPen(QPen(tokens.Border, 1));
    p.drawLine(0, height() - 1, width(), height() - 1);

    QWidget::paintEvent(event);
}

} // namespace Kirana
