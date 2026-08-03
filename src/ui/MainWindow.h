#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include <QTimer>
#include <QEvent>
#include "ui/Sidebar.h"
#include "core/AuthData.h"

class QLabel;  // ---- ADDED: API Integration ----

namespace Kirana {

class AppController;
class AuthController;
class Database;
class StatusBar;
class AuthWidget;
class DashboardWidget;
class DailyEntryWidget;
class StockWidget;
class AnalyticsWidget;
class ImportWidget;
class SettingsWidget;
class ProductCatalogWidget;
// No longer using PipelineWorker

// ─────────────────────────────────────────────
// MainWindow — application shell
//
// Auth flow:  AuthWidget (full screen) → MainWindow body
//
// Layout:
//   ┌── Sidebar(160px) ─┬── body ─────────────┐
//   │  Inventra logo    │  StatusBar (30px)    │
//   │  Nav items        │  QStackedWidget      │
//   │  (role-gated)     │    0: Dashboard      │
//   │                   │    1: Daily Entry    │
//   │                   │    2: Stock In/Out   │
//   │                   │    3: Products       │
//   │                   │    4: Analytics      │
//   │                   │    5: Import         │
//   │                   │    6: Settings       │
//   └───────────────────┴──────────────────────┘
// ─────────────────────────────────────────────

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(AppController* controller,
                        AuthController* auth,
                        QWidget* parent = nullptr);
    ~MainWindow() override;

    // Called after the auth flow completes
    void showAppShell();

protected:
    void closeEvent(QCloseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void onPageSelected(Sidebar::Page page);
    void onLoginSucceeded(const Kirana::StaffUser& user);
    void onLoggedOut();
    // PipelineWorker methods removed
    void onSessionLocked();
    void onSessionUnlocked();
    void onIdleTimeout();
    void onApiHealthChecked(bool isOnline);  // ---- ADDED: API Integration ----

private:
    void buildAuthLayer();
    void buildAppShell();
    void connectSignals();
    void showPage(int index);
    void resetIdleTimer();

    // Controllers (not owned)
    AppController*  m_controller    = nullptr;
    AuthController* m_auth          = nullptr;

    // Top-level stack: 0=auth, 1=app shell
    QStackedWidget* m_rootStack     = nullptr;

    // Auth
    AuthWidget*     m_authWidget    = nullptr;

    // App shell widgets
    QWidget*        m_appShell      = nullptr;
    Sidebar*        m_sidebar       = nullptr;
    StatusBar*      m_statusBar     = nullptr;
    QStackedWidget* m_stack         = nullptr;

    // Idle timer
    QTimer*         m_idleTimer     = nullptr;

    // Pages (indices match Sidebar::Page enum)
    DashboardWidget*       m_dashboardPage   = nullptr;
    DailyEntryWidget*      m_dailyEntryPage  = nullptr;
    StockWidget*           m_stockPage       = nullptr;
    ProductCatalogWidget*  m_productsPage    = nullptr;
    AnalyticsWidget*       m_analyticsPage   = nullptr;
    ImportWidget*          m_importPage      = nullptr;
    SettingsWidget*        m_settingsPage    = nullptr;

    // ---- ADDED: API Integration ----
    QLabel* m_apiBadge      = nullptr;
    QTimer* m_healthTimer   = nullptr;
    // ---- END ADDED ----
};

} // namespace Kirana
