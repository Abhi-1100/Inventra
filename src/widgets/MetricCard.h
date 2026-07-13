#pragma once

#include <QFrame>
#include <QString>
#include <QColor>
#include <QLabel>  // needed for inline setIconText

namespace Kirana {

// ─────────────────────────────────────────────
// MetricCard
//
// KPI card matching stitch_screens reference design:
// - Gradient dark background (#161b22 → #12161c)
// - Label (top-left, muted) + icon chip (top-right)
// - Large JetBrains Mono value with optional delta badge
// - Optional subtitle line
// - Top-border accent glow on hover
// ─────────────────────────────────────────────

class MetricCard : public QFrame {
    Q_OBJECT

public:
    explicit MetricCard(const QString& title,
                        const QColor&  accentColor,
                        QWidget* parent = nullptr);

    void setValue   (const QString& text);
    void setSubtitle(const QString& text);
    void setDelta   (const QString& delta, bool positive = true);
    void setAccent  (const QColor& color);

    // Set icon text shown in the chip (e.g. emoji, symbol letter)
    void setIconText(const QString& text);

protected:
    void paintEvent (QPaintEvent* event) override;
    void enterEvent (QEnterEvent* event) override;
    void leaveEvent (QEvent* event) override;

private:
    QLabel* m_titleLabel    = nullptr;
    QLabel* m_valueLabel    = nullptr;
    QLabel* m_subtitleLabel = nullptr;
    QLabel* m_deltaLabel    = nullptr;
    QLabel* m_iconLabel     = nullptr;

    QColor  m_accentColor;
    QString m_title;
    bool    m_hovered = false;

    void buildLayout();
};

} // namespace Kirana
