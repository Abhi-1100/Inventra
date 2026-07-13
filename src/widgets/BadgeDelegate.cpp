#include "widgets/BadgeDelegate.h"
#include "core/ProductModel.h"

#include <QPainter>
#include <QPainterPath>
#include <QApplication>
#include <QStyleOption>

namespace Kirana {

// Design system colors — matching stitch_screens Material Design 3 palette
static const QColor kPrimary        ("#afc6ff");  // safe/optimal
static const QColor kPrimaryContainer("#1f6feb"); // CTA / low stock indicator
static const QColor kError          ("#ffb4ab");  // critical low
static const QColor kTertiary       ("#c0c7d3");  // overstock / medium
static const QColor kSecondary      ("#acc7ff");  // secondary overstock
static const QColor kOutline        ("#8c90a0");  // muted / low demand
static const QColor kRowHover       ("#1c2025");
static const QColor kRowSelected    ("#1c2025");
static const QColor kRowBase        ("#101419");

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
// fillBackground
// ─────────────────────────────────────────────

void BadgeDelegate::fillBackground(QPainter* p,
                                   const QStyleOptionViewItem& opt) const
{
    QColor bg = kRowBase;

    if (opt.state & QStyle::State_Selected) {
        bg = kRowSelected;
    } else if (opt.state & QStyle::State_MouseOver) {
        bg = kRowHover;
    }

    p->fillRect(opt.rect, bg);

    // Bottom border 0.5px
    QColor border("#232a33");
    border.setAlphaF(0.5f);
    p->fillRect(opt.rect.left(), opt.rect.bottom(), opt.rect.width(), 1, border);
}

// ─────────────────────────────────────────────
// paintBadge — pill badges matching stitch_screens
// ─────────────────────────────────────────────

void BadgeDelegate::paintBadge(QPainter* painter,
                                const QStyleOptionViewItem& option,
                                const QModelIndex& index) const
{
    const QString text  = index.data(Qt::DisplayRole).toString();
    const QVariant cvar = index.data(ProductRole::BadgeColor);

    // ── Status dot + label variant ──────────────
    // Check if this is a status column (dot + text pattern)
    if (text == QLatin1String("Optimal") ||
        text == QLatin1String("Safe"))
    {
        paintStatusDot(painter, option, kPrimary, text, true);
        return;
    }
    if (text == QLatin1String("Critical") ||
        text == QLatin1String("Critical Low"))
    {
        paintStatusDot(painter, option, kError, text, false);
        return;
    }
    if (text == QLatin1String("Reorder") ||
        text == QLatin1String("Reorder Soon"))
    {
        paintStatusDot(painter, option, kTertiary, text, false);
        return;
    }
    if (text == QLatin1String("Overstock"))
    {
        paintStatusDot(painter, option, kSecondary, text, false);
        return;
    }

    // ── Standard pill badge ──────────────────────
    if (text.isEmpty() || text == QLatin1String("—")) return;

    QColor statusColor = kOutline;
    if (cvar.isValid()) {
        QColor c = cvar.value<QColor>();
        if (c.isValid()) statusColor = c;
    }

    // Determine badge style from text
    QColor bgColor;
    QColor borderColor;
    QColor textColor;

    QString upperText = text.toUpper();
    if (text == QLatin1String("High") || upperText == QLatin1String("HIGH")) {
        bgColor    = QColor(kPrimary); bgColor.setAlphaF(0.18f);
        borderColor= QColor(kPrimary); borderColor.setAlphaF(0.4f);
        textColor  = kPrimary;
    } else if (text == QLatin1String("Med") || text == QLatin1String("Medium") ||
               upperText == QLatin1String("MED")) {
        bgColor    = QColor(kTertiary); bgColor.setAlphaF(0.12f);
        borderColor= QColor(kOutline);  borderColor.setAlphaF(0.5f);
        textColor  = QColor("#e0e2ea");
    } else if (text == QLatin1String("Low") || upperText == QLatin1String("LOW")) {
        bgColor    = QColor(0,0,0,0); // transparent
        borderColor= QColor(kOutline); borderColor.setAlphaF(0.5f);
        textColor  = kOutline;
    } else {
        // generic: use the accent color
        bgColor    = statusColor; bgColor.setAlphaF(0.18f);
        borderColor= statusColor; borderColor.setAlphaF(0.35f);
        textColor  = statusColor;
    }

    // Badge font
    QFont badgeFont;
    badgeFont.setFamily(QStringLiteral("Hanken Grotesk"));
    badgeFont.setPixelSize(11);
    badgeFont.setWeight(QFont::DemiBold);
    badgeFont.setLetterSpacing(QFont::AbsoluteSpacing, 0.8);

    const QFontMetrics fm(badgeFont);
    const int padH   = 8;
    const int padV   = 3;
    const int badgeW = fm.horizontalAdvance(upperText) + padH * 2;
    const int badgeH = fm.height() + padV * 2;

    const QRect cellRect = option.rect;
    const int   badgeX   = cellRect.left() + 8;
    const int   badgeY   = cellRect.center().y() - badgeH / 2;
    const QRect badgeRect(badgeX, badgeY, badgeW, badgeH);

    painter->setRenderHint(QPainter::Antialiasing);

    // Background
    painter->setPen(QPen(borderColor, 1));
    painter->setBrush(bgColor);
    painter->drawRoundedRect(badgeRect.adjusted(0,0,-1,-1), 3, 3);

    // Text
    painter->setFont(badgeFont);
    painter->setPen(textColor);
    painter->drawText(badgeRect, Qt::AlignCenter, upperText);
}

// ─────────────────────────────────────────────
// paintStatusDot — dot + label (from stitch_screens)
// ─────────────────────────────────────────────

void BadgeDelegate::paintStatusDot(QPainter* p,
                                    const QStyleOptionViewItem& option,
                                    const QColor& dotColor,
                                    const QString& labelText,
                                    bool pulsing) const
{
    Q_UNUSED(pulsing); // Qt doesn't support CSS animations; static for now

    painter_helper(p, option, dotColor, labelText);
}

void BadgeDelegate::painter_helper(QPainter* p,
                                    const QStyleOptionViewItem& option,
                                    const QColor& dotColor,
                                    const QString& labelText) const
{
    const QRect r = option.rect;
    const int cx = r.left() + 12;
    const int cy = r.center().y();
    const int dotR = 4;

    p->setRenderHint(QPainter::Antialiasing);

    // Dot
    p->setPen(Qt::NoPen);
    p->setBrush(dotColor);
    p->drawEllipse(cx - dotR, cy - dotR, dotR*2, dotR*2);

    // Label
    QFont labelFont;
    labelFont.setFamily(QStringLiteral("Hanken Grotesk"));
    labelFont.setPixelSize(13);
    labelFont.setWeight(QFont::Normal);

    p->setFont(labelFont);
    p->setPen(dotColor.lighter(110));
    p->drawText(QRect(cx + dotR + 6, r.top(), r.right() - cx - dotR - 8, r.height()),
                Qt::AlignVCenter | Qt::AlignLeft, labelText);
}

// ─────────────────────────────────────────────
// paintForecast — trend arrow + numeric value
// ─────────────────────────────────────────────

void BadgeDelegate::paintForecast(QPainter* painter,
                                   const QStyleOptionViewItem& option,
                                   const QModelIndex& index) const
{
    const double trend   = index.data(ProductRole::TrendValue).toDouble();
    const QString numText= index.data(Qt::DisplayRole).toString() + QStringLiteral(" u");

    QString arrow;
    QColor  arrowColor;
    if      (trend >  0.25) { arrow = QStringLiteral("▲"); arrowColor = kPrimary; }
    else if (trend < -0.25) { arrow = QStringLiteral("▼"); arrowColor = kError; }
    else                     { arrow = QStringLiteral("→"); arrowColor = kOutline; }

    QFont monoFont;
    monoFont.setFamily(QStringLiteral("JetBrains Mono"));
    monoFont.setPixelSize(13);
    const QFontMetrics fm(monoFont);

    const QRect r  = option.rect.adjusted(-4, 0, -8, 0);
    const int   cy = r.center().y();

    painter->setRenderHint(QPainter::Antialiasing);
    painter->setFont(monoFont);

    const QString combined = arrow + QStringLiteral(" ") + numText;
    const int totalW = fm.horizontalAdvance(combined);
    const int startX = r.right() - totalW;

    QRect arrowRect(startX, cy - fm.height() / 2,
                    fm.horizontalAdvance(arrow + QStringLiteral(" ")),
                    fm.height());
    painter->setPen(arrowColor);
    painter->drawText(arrowRect, Qt::AlignLeft | Qt::AlignVCenter,
                      arrow + QStringLiteral(" "));

    QRect numRect(arrowRect.right(), cy - fm.height() / 2,
                  fm.horizontalAdvance(numText) + 4, fm.height());
    painter->setPen(QColor("#c2c6d6")); // on-surface-variant
    painter->drawText(numRect, Qt::AlignLeft | Qt::AlignVCenter, numText);
}

// ─────────────────────────────────────────────
// sizeHint
// ─────────────────────────────────────────────

QSize BadgeDelegate::sizeHint(const QStyleOptionViewItem& option,
                               const QModelIndex& /*index*/) const
{
    return QSize(option.rect.width(), 44);
}

} // namespace Kirana
