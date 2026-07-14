#pragma once

#include <QIcon>
#include <QIconEngine>
#include <QFont>
#include <QColor>
#include <QVariantMap>

namespace Kirana {

class QtAwesomeIconEngine : public QIconEngine {
public:
    QtAwesomeIconEngine(const QFont& font, char32_t character, const QColor& color = QColor())
        : m_font(font), m_character(character), m_color(color) {}

    void paint(QPainter* painter, const QRect& rect, QIcon::Mode mode, QIcon::State state) override;
    QIconEngine* clone() const override;

private:
    QFont m_font;
    char32_t m_character;
    QColor m_color;
};

class QtAwesome {
public:
    static QtAwesome& instance();

    enum IconCode : char32_t {
        Dashboard   = 0xf02c,
        DailyEntry  = 0xedfd,
        Stock       = 0xeaff,
        Products    = 0xea45,
        Analytics   = 0xea59,
        Import      = 0xedea,
        Settings    = 0xeb20,
        Search      = 0xeb1c,
        Bell        = 0xea35,
        User        = 0xeb4d
    };

    QIcon icon(char32_t character, const QColor& color = QColor());
    QPixmap pixmap(char32_t character, int size, const QColor& color = QColor());

private:
    QtAwesome();
    bool m_initialized = false;
    QFont m_font;
};

} // namespace Kirana
