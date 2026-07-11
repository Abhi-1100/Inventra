#include "widgets/MetricCard.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPaintEvent>

namespace Kirana {

MetricCard::MetricCard(const QString& title,
                       const QColor&  accentColor,
                       QWidget*       parent)
    : QFrame(parent)
    , m_accentColor(accentColor)
{
    setObjectName("MetricCard");
    setFixedHeight(112);
    setMinimumWidth(180);

    // Surface styling — matches .card QSS rule + 1px left accent painted manually
    setStyleSheet(
        "QFrame#MetricCard {"
        "  background: #161b22;"
        "  border: 1px solid #30363d;"
        "  border-radius: 8px;"
        "}"
    );

    buildLayout();
    setValue("—");
    setSubtitle("");
}

// ─────────────────────────────────────────────
// Layout
// ─────────────────────────────────────────────

void MetricCard::buildLayout() {
    // Outer wrapper gives space for the left accent bar (painted in paintEvent)
    auto* outerLayout = new QHBoxLayout(this);
    outerLayout->setContentsMargins(16, 12, 16, 12);
    outerLayout->setSpacing(0);

    auto* inner = new QWidget(this);
    inner->setStyleSheet("background: transparent;");
    auto* vlay = new QVBoxLayout(inner);
    vlay->setContentsMargins(0, 0, 0, 0);
    vlay->setSpacing(4);

    // Title
    m_titleLabel = new QLabel(this);
    m_titleLabel->setStyleSheet(
        "font-family: 'Segoe UI', 'Inter', sans-serif;"
        "font-size: 10px;"
        "font-weight: 600;"
        "letter-spacing: 1px;"
        "color: #8b949e;"
        "text-transform: uppercase;"
        "background: transparent;"
    );
    m_titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    // Value (large monospace number)
    m_valueLabel = new QLabel(this);
    m_valueLabel->setStyleSheet(
        "font-family: 'Consolas', 'JetBrains Mono', 'Courier New', monospace;"
        "font-size: 30px;"
        "font-weight: 700;"
        "color: #e6edf3;"
        "background: transparent;"
    );
    m_valueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    // Subtitle / delta
    m_subtitleLabel = new QLabel(this);
    m_subtitleLabel->setStyleSheet(
        "font-family: 'Segoe UI', 'Inter', sans-serif;"
        "font-size: 11px;"
        "color: #8b949e;"
        "background: transparent;"
    );
    m_subtitleLabel->setAlignment(Qt::AlignLeft | Qt::AlignBottom);

    vlay->addWidget(m_titleLabel);
    vlay->addWidget(m_valueLabel);
    vlay->addWidget(m_subtitleLabel);
    vlay->addStretch();

    outerLayout->addWidget(inner);
}

// ─────────────────────────────────────────────
// Setters
// ─────────────────────────────────────────────

void MetricCard::setValue(const QString& text) {
    m_valueLabel->setText(text);
    // Colour the value to match the accent
    m_valueLabel->setStyleSheet(
        QString("font-family: 'Consolas','JetBrains Mono','Courier New',monospace;"
                "font-size: 30px; font-weight: 700;"
                "color: %1; background: transparent;")
        .arg(m_accentColor.name())
    );
}

void MetricCard::setSubtitle(const QString& text) {
    m_subtitleLabel->setText(text);
    m_subtitleLabel->setVisible(!text.isEmpty());
}

void MetricCard::setAccent(const QColor& color) {
    m_accentColor = color;
    update();
}

void MetricCard::paintEvent(QPaintEvent* event) {
    QFrame::paintEvent(event);

    // Draw 3-px left accent bar inside border
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);
    p.setPen(Qt::NoPen);
    p.setBrush(m_accentColor);
    p.drawRect(1, 10, 3, height() - 20);
}

} // namespace Kirana
