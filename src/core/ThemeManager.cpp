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
    if (m_theme == Theme::Dark) {
        m_tokens.BgPrimary = QColor(QStringLiteral("#0b0f14"));
        m_tokens.BgSurface = QColor(QStringLiteral("#161b22"));
        m_tokens.BgOverlay = QColor(QStringLiteral("#21262d"));
        m_tokens.Border = QColor(QStringLiteral("#30363d"));
        m_tokens.TextPrimary = QColor(QStringLiteral("#e6edf3"));
        m_tokens.TextSecondary = QColor(QStringLiteral("#8b949e"));
        m_tokens.TextMuted = QColor(QStringLiteral("#6e7681"));
        m_tokens.Critical = QColor(QStringLiteral("#f85149"));
        m_tokens.Warning = QColor(QStringLiteral("#d29922"));
        m_tokens.Success = QColor(QStringLiteral("#3fb950"));
        m_tokens.Accent = QColor(QStringLiteral("#1f6feb"));
        m_tokens.Info = QColor(QStringLiteral("#1f6feb"));
        m_tokens.Gradient = QStringLiteral("qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #161b22, stop:1 #12161c)");

        QFile file(QStringLiteral(":/styles/terminal.qss"));
        if (file.open(QFile::ReadOnly | QFile::Text)) {
            qApp->setStyleSheet(QString::fromUtf8(file.readAll()));
        }
    } else {
        m_tokens.BgPrimary = QColor(QStringLiteral("#f8f9ff"));
        m_tokens.BgSurface = QColor(QStringLiteral("#ffffff"));
        m_tokens.BgOverlay = QColor(QStringLiteral("#f1f5f9"));
        m_tokens.Border = QColor(QStringLiteral("#e2e8f0"));
        m_tokens.TextPrimary = QColor(QStringLiteral("#0b1c30"));
        m_tokens.TextSecondary = QColor(QStringLiteral("#475569"));
        m_tokens.TextMuted = QColor(QStringLiteral("#94a3b8"));
        m_tokens.Critical = QColor(QStringLiteral("#ef4444"));
        m_tokens.Warning = QColor(QStringLiteral("#f59e0b"));
        m_tokens.Success = QColor(QStringLiteral("#10b981"));
        m_tokens.Accent = QColor(QStringLiteral("#0f172a"));
        m_tokens.Info = QColor(QStringLiteral("#3b82f6"));
        m_tokens.Gradient = QStringLiteral("qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #ffffff, stop:1 #f8fafc)");

        QFile file(QStringLiteral(":/styles/terminal_light.qss"));
        if (file.open(QFile::ReadOnly | QFile::Text)) {
            qApp->setStyleSheet(QString::fromUtf8(file.readAll()));
        }
    }
    emit themeChanged();
}

} // namespace Kirana
