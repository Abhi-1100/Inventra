#pragma once

#include <QStyledItemDelegate>
#include <QColor>

namespace Kirana {

// ─────────────────────────────────────────────
// BadgeDelegate
//
// Renders badge-type cells (Demand / Status /
// Priority columns) as pill badges:
//   – background: statusColor @ 22% opacity
//   – text:       statusColor @ 100% opacity
//   – shape:      rounded rect, 3px radius
//
// Also renders the Forecast column with a trend
// arrow glyph (▲ / → / ▼) before the number.
// ─────────────────────────────────────────────

class BadgeDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    enum class Mode {
        Badge,     // pill badge (Demand/Status/Priority columns)
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

    void paintBadge   (QPainter*, const QStyleOptionViewItem&, const QModelIndex&) const;
    void paintForecast(QPainter*, const QStyleOptionViewItem&, const QModelIndex&) const;

    void fillBackground(QPainter*, const QStyleOptionViewItem&) const;
};

} // namespace Kirana
