#include "widgets/MetricCard.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPaintEvent>
#include <QEnterEvent>
#include <QLinearGradient>

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
        "font-family: 'Hanken Grotesk', 'Segoe UI', sans-serif;"
        "font-size: 13px;"
        "font-weight: 600;"
        "letter-spacing: 0.02em;"
        "color: #8c90a0;"
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
        "font-family: 'JetBrains Mono', 'Consolas', monospace;"
        "font-size: 26px;"
        "font-weight: 500;"
        "color: #e0e2ea;"
        "background: transparent;"
        "border: none;");
    m_valueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    m_deltaLabel = new QLabel(this);
    m_deltaLabel->setStyleSheet(
        "font-family: 'JetBrains Mono', monospace;"
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
        "font-family: 'Hanken Grotesk', sans-serif;"
        "font-size: 12px;"
        "color: #8c90a0;"
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
    QString color = positive ? "#afc6ff" : "#ffb4ab";
    m_deltaLabel->setStyleSheet(
        QString("font-family:'JetBrains Mono',monospace;"
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

    // Card gradient background: #161b22 -> #12161c
    QLinearGradient bg(0, 0, 0, r.height());
    bg.setColorAt(0.0, QColor("#161b22"));
    bg.setColorAt(1.0, QColor("#12161c"));

    // Border
    p.setPen(QPen(QColor("#232a33"), 1));
    p.setBrush(bg);
    p.drawRoundedRect(r.adjusted(0, 0, -1, -1), 8, 8);

    // Hover top-border glow (1px colored top line)
    if (m_hovered) {
        QColor glow = m_accentColor;
        glow.setAlphaF(0.5f);
        p.setPen(QPen(glow, 1));
        p.drawLine(r.left() + 8, r.top(), r.right() - 8, r.top());
    }

    // Icon chip (colored rounded square behind m_iconLabel)
    if (m_iconLabel) {
        QRect iconBg = m_iconLabel->geometry().adjusted(-2, -2, 2, 2);
        QColor chipBg = m_accentColor;
        chipBg.setAlphaF(0.12f);
        p.setPen(Qt::NoPen);
        p.setBrush(chipBg);
        p.drawRoundedRect(iconBg, 6, 6);
    }

    // Update title label text
    if (m_titleLabel && m_titleLabel->text() != m_title) {
        m_titleLabel->setText(m_title);
    }

    // Colored value
    if (m_valueLabel) {
        m_valueLabel->setStyleSheet(
            QString("font-family:'JetBrains Mono','Consolas',monospace;"
                    "font-size:26px;font-weight:500;"
                    "color:#e0e2ea;"
                    "background:transparent;border:none;"));
    }
    // Accent icon text
    if (m_iconLabel) {
        m_iconLabel->setStyleSheet(
            QString("font-size:14px;color:%1;background:transparent;border:none;")
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
