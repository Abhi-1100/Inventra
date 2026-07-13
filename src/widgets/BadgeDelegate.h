#pragma once

#include <QStyledItemDelegate>
#include <QColor>

namespace Kirana {

// ─────────────────────────────────────────────
// BadgeDelegate
//
// Renders badge-type cells matching stitch_screens design:
//
// Badge mode:
//   - High demand: primary/20 bg + primary border + primary text
//   - Med demand:  tertiary/12 bg + outline border + on-surface text
//   - Low demand:  transparent + outline border + outline text
//   - Status dot+label: colored 8px dot + text
//
// Forecast mode:
//   - trend arrow (▲/→/▼) + EOQ quantity
// ─────────────────────────────────────────────

class BadgeDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    enum class Mode {
        Badge,     // pill badge / status dot (Demand/Status/Priority columns)
        Forecast   // trend arrow + number (Forecast column)
    };

    explicit BadgeDelegate(Mode mode, QObject* parent = nullptr);

    void paint(QPainter* painter,
               const QStyleOptionViewItem& option,
               const QModelIndex&          index) const override;

    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex&          index) const override;

private:
    Mode m_mode;

    void paintBadge     (QPainter*, const QStyleOptionViewItem&, const QModelIndex&) const;
    void paintForecast  (QPainter*, const QStyleOptionViewItem&, const QModelIndex&) const;
    void paintStatusDot (QPainter*, const QStyleOptionViewItem&,
                         const QColor& dotColor, const QString& label, bool pulsing) const;
    void painter_helper (QPainter*, const QStyleOptionViewItem&,
                         const QColor& dotColor, const QString& label) const;

    void fillBackground(QPainter*, const QStyleOptionViewItem&) const;
};

} // namespace Kirana
