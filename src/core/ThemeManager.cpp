#include "core/ThemeManager.h"
#include <QApplication>
#include <QFile>

namespace Kirana {

ThemeManager& ThemeManager::instance() {
    static ThemeManager instance;
    return instance;
}

ThemeManager::ThemeManager() : m_theme(Dark) {
    setTheme(Dark);
}

void ThemeManager::setTheme(Theme t) {
    m_theme = t;

    if (t == Dark) {
        // Material Design 3 dark palette — matching stitch_screens design tokens
        m_tokens.BgPrimary    = QColor("#101419");   // background
        m_tokens.BgSurface    = QColor("#161b22");   // card surface-low
        m_tokens.BgOverlay    = QColor("#1c2025");   // surface-container
        m_tokens.Border       = QColor("#232a33");   // card border
        m_tokens.TextPrimary  = QColor("#e0e2ea");   // on-surface
        m_tokens.TextSecondary= QColor("#c2c6d6");   // on-surface-variant
        m_tokens.TextMuted    = QColor("#8c90a0");   // outline
        m_tokens.Critical     = QColor("#ffb4ab");   // error
        m_tokens.Warning      = QColor("#c0c7d3");   // tertiary (overstock)
        m_tokens.Success      = QColor("#afc6ff");   // primary (safe stock)
        m_tokens.Accent       = QColor("#afc6ff");   // primary
        m_tokens.Info         = QColor("#acc7ff");   // secondary
        m_tokens.Gradient     = "qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #161b22, stop:1 #12161c)";

        QFile styleFile(QStringLiteral(":/styles/terminal.qss"));
        if (styleFile.open(QFile::ReadOnly | QFile::Text)) {
            qApp->setStyleSheet(QLatin1String(styleFile.readAll()));
        }
    } else {
        // Light Mode fallback
        m_tokens.BgPrimary    = QColor("#f8f9ff");
        m_tokens.BgSurface    = QColor("#ffffff");
        m_tokens.BgOverlay    = QColor("#ffffff");
        m_tokens.Border       = QColor("#e2e8f0");
        m_tokens.TextPrimary  = QColor("#0b1c30");
        m_tokens.TextSecondary= QColor("#45464d");
        m_tokens.TextMuted    = QColor("#76777d");
        m_tokens.Critical     = QColor("#ba1a1a");
        m_tokens.Warning      = QColor("#d29922");
        m_tokens.Success      = QColor("#006a61");
        m_tokens.Accent       = QColor("#0d9488");
        m_tokens.Info         = QColor("#0d9488");
        m_tokens.Gradient     = "#ffffff";

        QFile styleFile(QStringLiteral(":/styles/terminal_light.qss"));
        if (styleFile.open(QFile::ReadOnly | QFile::Text)) {
            qApp->setStyleSheet(QLatin1String(styleFile.readAll()));
        }
    }

    emit themeChanged();
}

} // namespace Kirana
