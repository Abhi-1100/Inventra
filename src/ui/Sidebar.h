#pragma once

#include <QWidget>
#include <QVector>

class QLabel;

namespace Kirana {

// ─────────────────────────────────────────────
// SidebarButton — item in the icon nav rail
// ─────────────────────────────────────────────

class SidebarButton : public QWidget {
    Q_OBJECT
    Q_PROPERTY(bool active READ isActive WRITE setActive)
    Q_PROPERTY(qreal hoverProgress READ hoverProgress WRITE setHoverProgress)
    Q_PROPERTY(qreal activeProgress READ activeProgress WRITE setActiveProgress)

public:
    explicit SidebarButton(char32_t iconChar,
                            const QString& label,
                            const QString& tooltip,
                            QWidget* parent = nullptr);

    bool isActive() const   { return m_active; }
    void setActive(bool v);
    void setVisible(bool v) { QWidget::setVisible(v); }

    qreal hoverProgress() const { return m_hoverProgress; }
    void setHoverProgress(qreal p) { m_hoverProgress = p; update(); }

    qreal activeProgress() const { return m_activeProgress; }
    void setActiveProgress(qreal p) { m_activeProgress = p; update(); }

signals:
    void clicked();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    void enterEvent(QEnterEvent* event) override;
#else
    void enterEvent(QEvent* event) override;
#endif
    void leaveEvent(QEvent* event) override;

private:
    char32_t m_iconChar;
    QString m_label;
    bool    m_active  = false;
    bool    m_hovered = false;

    qreal   m_hoverProgress = 0.0;
    qreal   m_activeProgress = 0.0;

    void animateHover(bool hover);
    void animateActive(bool active);
};

// ─────────────────────────────────────────────
// Sidebar — 160px icon+label nav rail
// ─────────────────────────────────────────────

class Sidebar : public QWidget {
    Q_OBJECT

public:
    enum class Page : int {
        Dashboard   = 0,
        DailyEntry  = 1,
        Stock       = 2,
        Products    = 3,
        Analytics   = 4,
        Import      = 5,
        Settings    = 6
    };

    explicit Sidebar(QWidget* parent = nullptr);

    void setActivePage(Page page);
    Page activePage() const { return m_activePage; }

    // Role-based visibility gating
    void setRole(const QString& role);   // "Owner" or "Staff"

signals:
    void pageSelected(Kirana::Sidebar::Page page);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QVector<SidebarButton*> m_buttons;
    Page m_activePage = Page::Dashboard;

    void buildLayout();
    void connectButton(SidebarButton* btn, Page page);
};

} // namespace Kirana

Q_DECLARE_METATYPE(Kirana::Sidebar::Page)
