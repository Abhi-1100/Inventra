#include "widgets/QtAwesome.h"
#include <QFontDatabase>
#include <QPainter>
#include <QDebug>

namespace Kirana {

void QtAwesomeIconEngine::paint(QPainter* painter, const QRect& rect, QIcon::Mode mode, QIcon::State state) {
    Q_UNUSED(state)
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::TextAntialiasing);

    QColor drawColor = m_color;
    if (!drawColor.isValid()) {
        drawColor = QColor("#8b949e"); // Default secondary text color
    }

    if (mode == QIcon::Disabled) {
        drawColor = QColor("#424754");
    } else if (mode == QIcon::Selected) {
        drawColor = QColor("#1f6feb"); // Accent color
    }

    painter->setPen(drawColor);

    QFont font = m_font;
    int size = qMin(rect.width(), rect.height());
    font.setPixelSize(size > 0 ? size : 24);
    painter->setFont(font);

    QString text = QString::fromUcs4(&m_character, 1);
    painter->drawText(rect, Qt::AlignCenter, text);
    painter->restore();
}

QIconEngine* QtAwesomeIconEngine::clone() const {
    return new QtAwesomeIconEngine(m_font, m_character, m_color);
}

QtAwesome& QtAwesome::instance() {
    static QtAwesome inst;
    return inst;
}

QtAwesome::QtAwesome() {
    int fontId = QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/tabler-icons.ttf"));
    if (fontId != -1) {
        QStringList families = QFontDatabase::applicationFontFamilies(fontId);
        if (!families.isEmpty()) {
            m_font.setFamily(families.first());
            m_initialized = true;
        }
    } else {
        qWarning() << "Failed to load Tabler Icons font from resources!";
    }
}

QIcon QtAwesome::icon(char32_t character, const QColor& color) {
    if (!m_initialized) {
        return QIcon();
    }
    return QIcon(new QtAwesomeIconEngine(m_font, character, color));
}

QPixmap QtAwesome::pixmap(char32_t character, int size, const QColor& color) {
    QIcon ic = icon(character, color);
    if (ic.isNull()) {
        return QPixmap();
    }
    return ic.pixmap(size, size);
}

} // namespace Kirana
