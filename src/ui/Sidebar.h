#pragma once

#include <QWidget>
#include <QButtonGroup>
#include <QVector>

class QToolButton;
class QLabel;

namespace Kirana {

// ─────────────────────────────────────────────
// SidebarButton — QToolButton with a left accent
// bar painted when the button is the active page
// ─────────────────────────────────────────────

class SidebarButton : public QWidget {
    Q_OBJECT
    Q_PROPERTY(bool active READ isActive WRITE setActive)

public:
    explicit SidebarButton(const QString& iconPath,
                            const QString& tooltip,
                            QWidget* parent = nullptr);

    bool isActive() const  { return m_active; }
    void setActive(bool v) { m_active = v; update(); }

signals:
    void clicked();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent*  event) override;

private:
    QString m_iconPath;
    bool    m_active = false;
    bool    m_hovered = false;
    QPixmap m_icon;
};

// ─────────────────────────────────────────────
// Sidebar — 56px icon nav rail
// ─────────────────────────────────────────────

class Sidebar : public QWidget {
    Q_OBJECT

public:
    enum class Page : int {
        Dashboard = 0,
        Products  = 1,
        Analytics = 2,
        Import    = 3,
        Settings  = 4
    };

    explicit Sidebar(QWidget* parent = nullptr);

    void setActivePage(Page page);
    Page activePage() const { return m_activePage; }

signals:
    void pageSelected(Kirana::Sidebar::Page page);

private:
    QVector<SidebarButton*> m_buttons;
    Page m_activePage = Page::Dashboard;

    void buildLayout();
    void connectButton(SidebarButton* btn, Page page);
};

} // namespace Kirana

Q_DECLARE_METATYPE(Kirana::Sidebar::Page)
