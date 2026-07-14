#include "widgets/BadgeDelegate.h"
#include "core/ProductModel.h"
#include "core/ThemeManager.h"

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
// fillBackground
// ─────────────────────────────────────────────

void BadgeDelegate::fillBackground(QPainter* p,
                                   const QStyleOptionViewItem& opt) const
{
    const auto& tokens = ThemeManager::instance().tokens();
    QColor bg = tokens.BgPrimary;

    if (opt.state & QStyle::State_Selected) {
        bg = tokens.BgOverlay;
    } else if (opt.state & QStyle::State_MouseOver) {
        bg = QColor("#161616"); // Obsidian bg_hover
    }

    p->fillRect(opt.rect, bg);

    // Bottom border 0.5px
    QColor border = tokens.Border;
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
    const auto& tokens = ThemeManager::instance().tokens();
    const QString text  = index.data(Qt::DisplayRole).toString();
    const QVariant cvar = index.data(ProductRole::BadgeColor);

    // ── Status dot + label variant ──────────────
    if (text == QLatin1String("Optimal") ||
        text == QLatin1String("Safe"))
    {
        paintStatusDot(painter, option, tokens.Success, text, true);
        return;
    }
    if (text == QLatin1String("Critical") ||
        text == QLatin1String("Critical Low"))
    {
        paintStatusDot(painter, option, tokens.Critical, text, false);
        return;
    }
    if (text == QLatin1String("Reorder") ||
        text == QLatin1String("Reorder Soon"))
    {
        paintStatusDot(painter, option, tokens.Warning, text, false);
        return;
    }
    if (text == QLatin1String("Overstock"))
    {
        paintStatusDot(painter, option, tokens.Warning, text, false);
        return;
    }

    // ── Standard pill badge ──────────────────────
    if (text.isEmpty() || text == QLatin1String("—")) return;

    QColor statusColor = tokens.TextMuted;
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
        bgColor    = QColor(tokens.Success); bgColor.setAlphaF(0.18f);
        borderColor= QColor(tokens.Success); borderColor.setAlphaF(0.4f);
        textColor  = tokens.Success;
    } else if (text == QLatin1String("Med") || text == QLatin1String("Medium") ||
               upperText == QLatin1String("MED")) {
        bgColor    = QColor(tokens.Warning); bgColor.setAlphaF(0.12f);
        borderColor= QColor(tokens.Warning);  borderColor.setAlphaF(0.5f);
        textColor  = tokens.TextPrimary;
    } else if (text == QLatin1String("Low") || upperText == QLatin1String("LOW")) {
        bgColor    = QColor(0,0,0,0);
        borderColor= QColor(tokens.TextMuted); borderColor.setAlphaF(0.5f);
        textColor  = tokens.TextMuted;
    } else {
        bgColor    = statusColor; bgColor.setAlphaF(0.18f);
        borderColor= statusColor; borderColor.setAlphaF(0.35f);
        textColor  = statusColor;
    }

    // Badge font
    QFont badgeFont;
    badgeFont.setFamily(QStringLiteral("SF Mono"));
    badgeFont.setPixelSize(11);
    badgeFont.setWeight(QFont::Bold);
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
    labelFont.setFamily(QStringLiteral("SF Mono"));
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
    const auto& tokens = ThemeManager::instance().tokens();
    const double trend   = index.data(ProductRole::TrendValue).toDouble();
    const QString numText= index.data(Qt::DisplayRole).toString() + QStringLiteral(" u");

    QString arrow;
    QColor  arrowColor;
    if      (trend >  0.25) { arrow = QStringLiteral("▲"); arrowColor = tokens.Success; }
    else if (trend < -0.25) { arrow = QStringLiteral("▼"); arrowColor = tokens.Critical; }
    else                     { arrow = QStringLiteral("→"); arrowColor = tokens.TextMuted; }

    QFont monoFont;
    monoFont.setFamily(QStringLiteral("SF Mono"));
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
    painter->setPen(tokens.TextSecondary);
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
