#include "core/ThemeManager.h"
#include <QApplication>
#include <QFile>
#include <QDialog>
#include <QPropertyAnimation>
#include <QEvent>
#include <QEasingCurve>
#include <QGraphicsDropShadowEffect>

namespace Kirana {

class DialogAnimationFilter : public QObject {
protected:
    bool eventFilter(QObject* obj, QEvent* event) override {
        if (event->type() == QEvent::Show) {
            if (auto* dialog = qobject_cast<QDialog*>(obj)) {
                if (!dialog->property("animating").toBool()) {
                    dialog->setProperty("animating", true);

                    // 1. Fade-in
                    dialog->setWindowOpacity(0.0);
                    auto* fade = new QPropertyAnimation(dialog, "windowOpacity", dialog);
                    fade->setDuration(200);
                    fade->setStartValue(0.0);
                    fade->setEndValue(1.0);
                    fade->setEasingCurve(QEasingCurve::OutCubic);
                    fade->start(QAbstractAnimation::DeleteWhenStopped);

                    // 2. Scale-in
                    QRect target = dialog->geometry();
                    int cx = target.center().x();
                    int cy = target.center().y();
                    int w = target.width();
                    int h = target.height();

                    QRect start(cx - (w * 0.95) / 2, cy - (h * 0.95) / 2, w * 0.95, h * 0.95);
                    dialog->setGeometry(start);

                    auto* scale = new QPropertyAnimation(dialog, "geometry", dialog);
                    scale->setDuration(200);
                    scale->setStartValue(start);
                    scale->setEndValue(target);
                    scale->setEasingCurve(QEasingCurve::OutCubic);
                    scale->start(QAbstractAnimation::DeleteWhenStopped);
                }
            }
        }
        return QObject::eventFilter(obj, event);
    }
};

void ThemeManager::applyDropShadow(QWidget* widget, qreal blurRadius, const QColor& color) {
    if (!widget) return;
    auto* shadow = new QGraphicsDropShadowEffect(widget);
    shadow->setBlurRadius(blurRadius);
    shadow->setXOffset(0);
    shadow->setYOffset(4);
    shadow->setColor(color);
    widget->setGraphicsEffect(shadow);
}

ThemeManager& ThemeManager::instance() {
    static ThemeManager instance;
    return instance;
}

ThemeManager::ThemeManager() : m_theme(Dark) {
    setTheme(Dark);
    if (qApp) {
        qApp->installEventFilter(new DialogAnimationFilter());
    }
}

void ThemeManager::setTheme(Theme t) {
    m_theme = t;

    if (t == Dark) {
        // Obsidian Dark Theme
        m_tokens.BgPrimary    = QColor("#080808");   // Obsidian base BG
        m_tokens.BgSurface    = QColor("#0a0a0a");   // Obsidian surface
        m_tokens.BgOverlay    = QColor("#111111");   // Obsidian raised
        m_tokens.Border       = QColor("#222222");   // Obsidian border
        m_tokens.TextPrimary  = QColor("#e5e5e5");   // primary text
        m_tokens.TextSecondary= QColor("#808080");   // secondary text
        m_tokens.TextMuted    = QColor("#525252");   // muted/tertiary text
        m_tokens.Critical     = QColor("#dc2626");   // negative/reorder red
        m_tokens.Warning      = QColor("#d97706");   // warning/overstock amber
        m_tokens.Success      = QColor("#16a34a");   // positive/safe stock green
        m_tokens.Accent       = QColor("#d97706");   // amber accent
        m_tokens.Info         = QColor("#808080");   // info secondary
        m_tokens.Gradient     = "qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #0a0a0a, stop:1 #080808)";

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
