#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include "ui/Sidebar.h"

namespace Kirana {

class AppController;
class StatusBar;
class DashboardWidget;
class AnalyticsWidget;
class ImportWidget;
class SettingsWidget;
class PipelineWorker;
struct PipelineRunResult;


// ─────────────────────────────────────────────
// MainWindow — application shell
//
// Layout:
//   ┌── Sidebar(56px) ─┬── body ────────────┐
//   │                  │  StatusBar (30px)   │
//   │    Icon nav      │  QStackedWidget     │
//   │                  │    0: Dashboard     │
//   │                  │    1: Products      │
//   │                  │    2: Analytics     │
//   │                  │    3: Import        │
//   │                  │    4: Settings      │
//   └──────────────────┴─────────────────────┘
// ─────────────────────────────────────────────

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(AppController* controller, QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onPageSelected(Sidebar::Page page);
    void onRunNowClicked();
    void onPipelineStarted();
    void onPipelineProgress(int pct, const QString& stage);
    void onPipelineFinished(const PipelineRunResult& result);
    void onPipelineError(const QString& msg);

private:
    void buildLayout();
    void connectSignals();
    void showPage(int index);

    // Controller (not owned)
    AppController* m_controller = nullptr;

    // Shell widgets
    Sidebar*        m_sidebar    = nullptr;
    StatusBar*      m_statusBar  = nullptr;
    QStackedWidget* m_stack      = nullptr;

    // Pages
    DashboardWidget* m_dashboardPage  = nullptr;
    DashboardWidget* m_productsPage   = nullptr;  // reuses DashboardWidget
    AnalyticsWidget* m_analyticsPage  = nullptr;
    ImportWidget*    m_importPage     = nullptr;
    SettingsWidget*  m_settingsPage   = nullptr;

    // Background pipeline worker
    PipelineWorker* m_worker = nullptr;
};

} // namespace Kirana
