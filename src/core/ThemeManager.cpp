#include "core/ThemeManager.h"
#include <QApplication>
#include <QFile>

namespace Kirana {

ThemeManager& ThemeManager::instance() {
    static ThemeManager inst;
    return inst;
}

ThemeManager::ThemeManager() : m_theme(Theme::Dark) {
    setTheme(Theme::Dark);
}

void ThemeManager::setTheme(Theme t) {
    m_theme = t;

    if (t == Theme::Dark) {
        // Material Design 3 dark palette — matching stitch_screens design tokens
        m_tokens.BgPrimary    = QColor(QStringLiteral("#101419"));   // background
        m_tokens.BgSurface    = QColor(QStringLiteral("#161b22"));   // card surface-low
        m_tokens.BgOverlay    = QColor(QStringLiteral("#1c2025"));   // surface-container
        m_tokens.Border       = QColor(QStringLiteral("#232a33"));   // card border
        m_tokens.TextPrimary  = QColor(QStringLiteral("#e0e2ea"));   // on-surface
        m_tokens.TextSecondary= QColor(QStringLiteral("#c2c6d6"));   // on-surface-variant
        m_tokens.TextMuted    = QColor(QStringLiteral("#8c90a0"));   // outline
        m_tokens.Critical     = QColor(QStringLiteral("#ffb4ab"));   // error
        m_tokens.Warning      = QColor(QStringLiteral("#c0c7d3"));   // tertiary (overstock)
        m_tokens.Success      = QColor(QStringLiteral("#afc6ff"));   // primary (safe stock)
        m_tokens.Accent       = QColor(QStringLiteral("#afc6ff"));   // primary
        m_tokens.Info         = QColor(QStringLiteral("#acc7ff"));   // secondary
        m_tokens.Gradient     = QStringLiteral("qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #161b22, stop:1 #12161c)");

        QFile styleFile(QStringLiteral(":/styles/terminal.qss"));
        if (styleFile.open(QFile::ReadOnly | QFile::Text)) {
            qApp->setStyleSheet(QString::fromUtf8(styleFile.readAll()));
        }
    } else {
        // Light Mode fallback
        m_tokens.BgPrimary    = QColor(QStringLiteral("#f8f9ff"));
        m_tokens.BgSurface    = QColor(QStringLiteral("#ffffff"));
        m_tokens.BgOverlay    = QColor(QStringLiteral("#ffffff"));
        m_tokens.Border       = QColor(QStringLiteral("#e2e8f0"));
        m_tokens.TextPrimary  = QColor(QStringLiteral("#0b1c30"));
        m_tokens.TextSecondary= QColor(QStringLiteral("#475569"));
        m_tokens.TextMuted    = QColor(QStringLiteral("#94a3b8"));
        m_tokens.Critical     = QColor(QStringLiteral("#ef4444"));
        m_tokens.Warning      = QColor(QStringLiteral("#f59e0b"));
        m_tokens.Success      = QColor(QStringLiteral("#10b981"));
        m_tokens.Accent       = QColor(QStringLiteral("#0f172a"));
        m_tokens.Info         = QColor(QStringLiteral("#3b82f6"));
        m_tokens.Gradient     = QStringLiteral("#ffffff");

        QFile styleFile(QStringLiteral(":/styles/terminal_light.qss"));
        if (styleFile.open(QFile::ReadOnly | QFile::Text)) {
            qApp->setStyleSheet(QString::fromUtf8(styleFile.readAll()));
        }
    }

    emit themeChanged();
}

} // namespace Kirana
