#pragma once

#include <QFrame>
#include <QString>
#include <QColor>

class QLabel;

namespace Kirana {

// ─────────────────────────────────────────────
// MetricCard
//
// A dark-surface KPI card used in the Dashboard.
// Shows a title, a large primary value, and an
// optional delta/subtitle.
//
// Left edge has a 3-px accent bar in a status
// colour (passed as accentColor).
// ─────────────────────────────────────────────

class MetricCard : public QFrame {
    Q_OBJECT

public:
    explicit MetricCard(const QString& title,
                        const QColor&  accentColor,
                        QWidget* parent = nullptr);

    void setValue   (const QString& text);
    void setSubtitle(const QString& text);
    void setAccent  (const QColor& color);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QLabel* m_titleLabel    = nullptr;
    QLabel* m_valueLabel    = nullptr;
    QLabel* m_subtitleLabel = nullptr;
    QColor  m_accentColor;

    void buildLayout();
};

} // namespace Kirana
