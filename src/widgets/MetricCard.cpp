#include "widgets/MetricCard.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPaintEvent>
#include <QEnterEvent>
#include <QLinearGradient>
#include <QGraphicsDropShadowEffect>
#include "core/ThemeManager.h"

namespace Kirana {

MetricCard::MetricCard(const QString& title,
                       const QColor&  accentColor,
                       QWidget*       parent)
    : QFrame(parent)
    , m_accentColor(accentColor)
    , m_title(title)
{
    setObjectName("MetricCard");
    setMinimumHeight(110);
    setMinimumWidth(180);
    setAttribute(Qt::WA_Hover);

    buildLayout();
    setValue(QStringLiteral("—"));
    setSubtitle(QString());

    auto* shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(20);
    shadow->setXOffset(0);
    shadow->setYOffset(4);
    shadow->setColor(QColor(31, 111, 235, 30)); // Subtle blue-tinted shadow
    setGraphicsEffect(shadow);
}

void MetricCard::setIconText(const QString& text) {
    if (m_iconLabel) {
        m_iconLabel->setText(text);
    }
}

// ─────────────────────────────────────────────
// Layout
// ─────────────────────────────────────────────

void MetricCard::buildLayout() {
    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(20, 16, 20, 16);
    outerLayout->setSpacing(6);

    // ── Row 1: Title + Icon chip ───────────────
    auto* topRow = new QHBoxLayout;
    topRow->setSpacing(8);

    m_titleLabel = new QLabel(this);
    m_titleLabel->setStyleSheet(
        "font-family: 'SF Mono', 'Menlo', 'Cascadia Mono', 'Consolas', monospace;"
        "font-size: 11px;"
        "font-weight: bold;"
        "letter-spacing: 0.05em;"
        "color: #808080;"
        "background: transparent;"
        "border: none;");
    m_titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    // Icon chip (colored circle on right)
    m_iconLabel = new QLabel(this);
    m_iconLabel->setFixedSize(28, 28);
    m_iconLabel->setAlignment(Qt::AlignCenter);

    topRow->addWidget(m_titleLabel);
    topRow->addStretch();
    topRow->addWidget(m_iconLabel);

    // ── Row 2: Value + Delta ───────────────────
    auto* valueRow = new QHBoxLayout;
    valueRow->setSpacing(10);
    valueRow->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    m_valueLabel = new QLabel(this);
    m_valueLabel->setStyleSheet(
        "font-family: 'SF Mono', 'Menlo', 'Cascadia Mono', 'Consolas', monospace;"
        "font-size: 26px;"
        "font-weight: bold;"
        "color: #e5e5e5;"
        "background: transparent;"
        "border: none;");
    m_valueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    m_deltaLabel = new QLabel(this);
    m_deltaLabel->setStyleSheet(
        "font-family: 'SF Mono', 'Menlo', 'Cascadia Mono', 'Consolas', monospace;"
        "font-size: 12px;"
        "font-weight: 400;"
        "background: transparent;"
        "border: none;");
    m_deltaLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_deltaLabel->setVisible(false);

    valueRow->addWidget(m_valueLabel);
    valueRow->addWidget(m_deltaLabel);
    valueRow->addStretch();

    // ── Row 3: Subtitle ───────────────────────
    m_subtitleLabel = new QLabel(this);
    m_subtitleLabel->setStyleSheet(
        "font-family: 'SF Mono', 'Menlo', 'Cascadia Mono', 'Consolas', monospace;"
        "font-size: 12px;"
        "color: #808080;"
        "background: transparent;"
        "border: none;");
    m_subtitleLabel->setAlignment(Qt::AlignLeft);
    m_subtitleLabel->setVisible(false);

    outerLayout->addLayout(topRow);
    outerLayout->addLayout(valueRow);
    outerLayout->addWidget(m_subtitleLabel);
    outerLayout->addStretch();
}

// ─────────────────────────────────────────────
// Setters
// ─────────────────────────────────────────────

void MetricCard::setValue(const QString& text) {
    m_valueLabel->setText(text);
}

void MetricCard::setSubtitle(const QString& text) {
    m_subtitleLabel->setText(text);
    m_subtitleLabel->setVisible(!text.isEmpty());
}

void MetricCard::setDelta(const QString& delta, bool positive) {
    m_deltaLabel->setText(delta);
    QString color = positive ? ThemeManager::instance().tokens().Success.name() : ThemeManager::instance().tokens().Critical.name();
    m_deltaLabel->setStyleSheet(
        QString("font-family:'SF Mono','Menlo','Cascadia Mono','Consolas',monospace;"
                "font-size:12px;font-weight:400;"
                "color:%1;background:transparent;border:none;").arg(color));
    m_deltaLabel->setVisible(!delta.isEmpty());
}

void MetricCard::setAccent(const QColor& color) {
    m_accentColor = color;
    update();
}

// ─────────────────────────────────────────────
// paintEvent — gradient background + top hover glow
// ─────────────────────────────────────────────

void MetricCard::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRect r = rect();
    const auto& tokens = ThemeManager::instance().tokens();

    // Card gradient background
    QLinearGradient bg(0, 0, 0, r.height());
    bg.setColorAt(0.0, tokens.BgSurface);
    bg.setColorAt(1.0, tokens.BgPrimary);

    // Border
    p.setPen(QPen(tokens.Border, 1));
    p.setBrush(bg);
    p.drawRoundedRect(r.adjusted(0, 0, -1, -1), 4, 4);

    // Hover top-border glow
    if (m_hovered) {
        QColor glow = m_accentColor;
        glow.setAlphaF(0.5f);
        p.setPen(QPen(glow, 1));
        p.drawLine(r.left() + 4, r.top(), r.right() - 4, r.top());
    }

    // Icon chip
    if (m_iconLabel) {
        QRect iconBg = m_iconLabel->geometry().adjusted(-2, -2, 2, 2);
        QColor chipBg = m_accentColor;
        chipBg.setAlphaF(0.12f);
        p.setPen(Qt::NoPen);
        p.setBrush(chipBg);
        p.drawRoundedRect(iconBg, 3, 3);
    }

    // Update title label text
    if (m_titleLabel && m_titleLabel->text() != m_title) {
        m_titleLabel->setText(m_title);
    }

    // Colored value
    if (m_valueLabel) {
        m_valueLabel->setStyleSheet(
            QString("font-family:'SF Mono','Menlo','Cascadia Mono','Consolas',monospace;"
                    "font-size:26px;font-weight:bold;"
                    "color:%1;"
                    "background:transparent;border:none;").arg(tokens.TextPrimary.name()));
    }
    // Accent icon text
    if (m_iconLabel) {
        m_iconLabel->setStyleSheet(
            QString("font-family:'SF Mono','Menlo','Cascadia Mono','Consolas',monospace;"
                    "font-size:14px;color:%1;background:transparent;border:none;")
            .arg(m_accentColor.name()));
    }
}

void MetricCard::enterEvent(QEnterEvent* ev) {
    m_hovered = true;
    update();
    QFrame::enterEvent(ev);
}

void MetricCard::leaveEvent(QEvent* ev) {
    m_hovered = false;
    update();
    QFrame::leaveEvent(ev);
}

} // namespace Kirana
