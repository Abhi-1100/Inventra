#include "widgets/BadgeDelegate.h"
#include "core/ProductModel.h"   // ProductRole constants

#include <QPainter>
#include <QPainterPath>
#include <QApplication>
#include <QStyleOption>

namespace Kirana {

BadgeDelegate::BadgeDelegate(Mode mode, QObject* parent)
    : QStyledItemDelegate(parent)
    , m_mode(mode)
{}

// ─────────────────────────────────────────────
// paint
// ─────────────────────────────────────────────

void BadgeDelegate::paint(QPainter* painter,
                          const QStyleOptionViewItem& option,
                          const QModelIndex& index) const
{
    painter->save();
    fillBackground(painter, option);

    if (m_mode == Mode::Badge)
        paintBadge(painter, option, index);
    else
        paintForecast(painter, option, index);

    painter->restore();
}

// ─────────────────────────────────────────────
// fillBackground — row hover / selection
// ─────────────────────────────────────────────

void BadgeDelegate::fillBackground(QPainter* p,
                                    const QStyleOptionViewItem& opt) const
{
    if (opt.state & QStyle::State_Selected) {
        p->fillRect(opt.rect, QColor("#1c2128"));
    } else if (opt.state & QStyle::State_MouseOver) {
        p->fillRect(opt.rect, QColor("#161b22"));
    } else {
        // alternate rows
        const int row = opt.index.row();
        p->fillRect(opt.rect,
            row % 2 == 0 ? QColor("#0d1117") : QColor("#0f1318"));
    }

    // Selected row: left accent bar
    if (opt.state & QStyle::State_Selected) {
        p->fillRect(opt.rect.left(), opt.rect.top(), 2, opt.rect.height(),
                    QColor("#58a6ff"));
    }
}

// ─────────────────────────────────────────────
// paintBadge — pill badge rendering
// ─────────────────────────────────────────────

void BadgeDelegate::paintBadge(QPainter* painter,
                                const QStyleOptionViewItem& option,
                                const QModelIndex& index) const
{
    const QString text  = index.data(Qt::DisplayRole).toString();
    const QVariant cvar = index.data(ProductRole::BadgeColor);
    if (text.isEmpty() || text == "—" || !cvar.isValid()) return;

    const QColor statusColor = cvar.value<QColor>();
    if (!statusColor.isValid()) return;

    // Monospace font for badge text
    QFont badgeFont = option.font;
    badgeFont.setFamily("Consolas, 'JetBrains Mono', monospace");
    badgeFont.setPointSize(9);
    badgeFont.setWeight(QFont::DemiBold);

    const QFontMetrics fm(badgeFont);
    const int textW  = fm.horizontalAdvance(text);
    const int padH   = 10;
    const int padV   = 4;
    const int badgeW = textW + padH * 2;
    const int badgeH = fm.height() + padV * 2;

    const QRect cellRect  = option.rect;
    const int   badgeX    = cellRect.left() + 8;
    const int   badgeY    = cellRect.center().y() - badgeH / 2;
    const QRect badgeRect(badgeX, badgeY, badgeW, badgeH);

    // Background fill — 22% opacity
    QColor bg = statusColor;
    bg.setAlphaF(0.22f);

    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(Qt::NoPen);
    painter->setBrush(bg);
    painter->drawRoundedRect(badgeRect, 3, 3);

    // Text — full opacity status colour
    painter->setPen(statusColor);
    painter->setFont(badgeFont);
    painter->drawText(badgeRect, Qt::AlignCenter, text);
}

// ─────────────────────────────────────────────
// paintForecast — trend arrow + numeric value
// ─────────────────────────────────────────────

void BadgeDelegate::paintForecast(QPainter* painter,
                                   const QStyleOptionViewItem& option,
                                   const QModelIndex& index) const
{
    const double trend = index.data(ProductRole::TrendValue).toDouble();
    const QString numText = index.data(Qt::DisplayRole).toString() + " u";

    // Trend indicator
    QString arrow;
    QColor  arrowColor;
    if      (trend >  0.25) { arrow = "▲"; arrowColor = QColor("#3fb950"); }
    else if (trend < -0.25) { arrow = "▼"; arrowColor = QColor("#f85149"); }
    else                     { arrow = "→"; arrowColor = QColor("#8b949e"); }

    QFont monoFont = option.font;
    monoFont.setFamily("Consolas, 'JetBrains Mono', monospace");
    monoFont.setPointSize(10);
    const QFontMetrics fm(monoFont);

    const QRect r = option.rect.adjusted(-4, 0, -8, 0);
    const int   cy = r.center().y();

    // Draw arrow
    const QString arrowStr = arrow + " ";
    const int arrowW = fm.horizontalAdvance(arrowStr);

    painter->setRenderHint(QPainter::Antialiasing);
    painter->setFont(monoFont);
    painter->setPen(arrowColor);

    QRect arrowRect(r.right() - fm.horizontalAdvance(arrowStr + numText),
                    cy - fm.height() / 2,
                    arrowW,
                    fm.height());
    painter->drawText(arrowRect, Qt::AlignLeft | Qt::AlignVCenter, arrowStr);

    // Draw number in primary text colour
    painter->setPen(QColor("#e6edf3"));
    QRect numRect(arrowRect.right(),
                  cy - fm.height() / 2,
                  fm.horizontalAdvance(numText) + 4,
                  fm.height());
    painter->drawText(numRect, Qt::AlignLeft | Qt::AlignVCenter, numText);
}

// ─────────────────────────────────────────────
// sizeHint
// ─────────────────────────────────────────────

QSize BadgeDelegate::sizeHint(const QStyleOptionViewItem& option,
                               const QModelIndex& /*index*/) const
{
    return QSize(option.rect.width(), 36);
}

} // namespace Kirana
