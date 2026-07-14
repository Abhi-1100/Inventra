#pragma once
#include <QObject>
#include <QColor>
#include <QWidget>

namespace Kirana {

struct ThemeTokens {
    QColor BgPrimary;
    QColor BgSurface;
    QColor BgOverlay;
    QColor Border;
    QColor TextPrimary;
    QColor TextSecondary;
    QColor TextMuted;
    QColor Critical;
    QColor Warning;
    QColor Success;
    QColor Accent;
    QColor Info;
    QString Gradient; // Background gradient for surface
};

class ThemeManager : public QObject {
    Q_OBJECT
public:
    enum Theme { Dark, Light };

    static ThemeManager& instance();

    void setTheme(Theme t);
    Theme theme() const { return m_theme; }
    const ThemeTokens& tokens() const { return m_tokens; }

    static void applyDropShadow(QWidget* widget, qreal blurRadius = 20.0, const QColor& color = QColor(31, 111, 235, 30));

signals:
    void themeChanged();

private:
    ThemeManager();
    Theme m_theme;
    ThemeTokens m_tokens;
};

} // namespace Kirana
