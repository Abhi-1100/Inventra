#include "ui/MainWindow.h"
#include "ui/Sidebar.h"
#include "ui/StatusBar.h"
#include "ui/DashboardWidget.h"
#include "ui/AnalyticsWidget.h"
#include "ui/ImportWidget.h"
#include "ui/SettingsWidget.h"
#include "core/AppController.h"
#include "core/PipelineWorker.h"
#include "python/PipelineResult.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QCloseEvent>
#include <QResizeEvent>
#include <QMessageBox>
#include <QStatusBar>

namespace Kirana {

MainWindow::MainWindow(AppController* controller, QWidget* parent)
    : QMainWindow(parent)
    , m_controller(controller)
{
    setObjectName("MainWindow");
    setWindowTitle("Kirana Terminal  v1.0");
    setMinimumSize(1200, 760);

    // Hide the Qt built-in status bar (we use our own top strip)
    statusBar()->hide();

    buildLayout();
    connectSignals();

    // Initial data load
    onPageSelected(Sidebar::Page::Dashboard);
}

MainWindow::~MainWindow() {
    if (m_worker && m_worker->isRunning()) {
        m_worker->requestStop();
        m_worker->wait(3000);
    }
}

// ─────────────────────────────────────────────
// buildLayout
// ─────────────────────────────────────────────

void MainWindow::buildLayout() {
    auto* central = new QWidget(this);
    central->setObjectName("CentralWidget");
    setCentralWidget(central);

    auto* rootLayout = new QHBoxLayout(central);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // ── Left: Sidebar ─────────────────────────
    m_sidebar = new Sidebar(this);
    rootLayout->addWidget(m_sidebar);

    // ── Right: body column ────────────────────
    auto* body = new QWidget(central);
    body->setObjectName("Body");
    auto* bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(0);

    // Top status strip
    m_statusBar = new StatusBar(body);
    bodyLayout->addWidget(m_statusBar);

    // Page stack
    m_stack = new QStackedWidget(body);
    m_stack->setObjectName("PageStack");

    m_dashboardPage = new DashboardWidget(m_controller, body);
    m_productsPage  = new DashboardWidget(m_controller, body);   // same widget, different instance
    m_analyticsPage = new AnalyticsWidget(m_controller, body);
    m_importPage    = new ImportWidget(m_controller, body);
    m_settingsPage  = new SettingsWidget(m_controller, body);

    m_stack->addWidget(m_dashboardPage);  // page index 0
    m_stack->addWidget(m_productsPage);   // page index 1
    m_stack->addWidget(m_analyticsPage);  // page index 2
    m_stack->addWidget(m_importPage);     // page index 3
    m_stack->addWidget(m_settingsPage);   // page index 4

    bodyLayout->addWidget(m_stack, 1);
    rootLayout->addWidget(body, 1);

    // Pipeline worker (lives on main thread; its run() is on a QThread)
    m_worker = new PipelineWorker(this);
}

// ─────────────────────────────────────────────
// connectSignals
// ─────────────────────────────────────────────

void MainWindow::connectSignals() {
    // Sidebar → page switch
    connect(m_sidebar, &Sidebar::pageSelected,
            this, &MainWindow::onPageSelected);

    // Status bar → run pipeline
    connect(m_statusBar, &StatusBar::runNowRequested,
            this, &MainWindow::onRunNowClicked);

    // Import page → run pipeline with a CSV
    connect(m_importPage, &ImportWidget::pipelineRunRequested,
            this, [this](const QString& csvPath) {
                m_worker->setCsvPath(csvPath);
                m_worker->setSettings(m_controller->settings());
                if (!m_worker->isRunning()) m_worker->start();
            });

    // Settings page → update settings
    connect(m_settingsPage, &SettingsWidget::settingsChanged,
            m_controller, &AppController::updateSettings);
    connect(m_settingsPage, &SettingsWidget::reRunRequested,
            this, &MainWindow::onRunNowClicked);

    // Worker → UI (all auto-queued because worker runs on different thread)
    connect(m_worker, &PipelineWorker::pipelineStarted,
            this, &MainWindow::onPipelineStarted, Qt::QueuedConnection);
    connect(m_worker, &PipelineWorker::progressUpdate,
            this, &MainWindow::onPipelineProgress, Qt::QueuedConnection);
    connect(m_worker, &PipelineWorker::pipelineFinished,
            this, &MainWindow::onPipelineFinished, Qt::QueuedConnection);
    connect(m_worker, &PipelineWorker::pipelineError,
            this, &MainWindow::onPipelineError, Qt::QueuedConnection);

    // Controller → status bar refresh
    connect(m_controller, &AppController::pipelineStateChanged,
            m_statusBar, &StatusBar::setPipelineState);
}

// ─────────────────────────────────────────────
// Slots
// ─────────────────────────────────────────────

void MainWindow::onPageSelected(Sidebar::Page page) {
    showPage(static_cast<int>(page));
}

void MainWindow::showPage(int index) {
    m_stack->setCurrentIndex(index);
    m_sidebar->setActivePage(static_cast<Sidebar::Page>(index));
}

void MainWindow::onRunNowClicked() {
    if (m_worker->isRunning()) return;  // already running
    m_worker->setCsvPath(QString());    // empty = use cached / dummy
    m_worker->setSettings(m_controller->settings());
    m_worker->start();
}

void MainWindow::onPipelineStarted() {
    m_statusBar->setPipelineState(true, m_controller->lastRunTime());
}

void MainWindow::onPipelineProgress(int /*pct*/, const QString& /*stage*/) {
    // Could update a progress indicator if added later
}

void MainWindow::onPipelineFinished(const PipelineRunResult& result) {
    if (result.success && !result.results.isEmpty()) {
        // Phase 2: applyPipelineResults would go here
        // For Phase 1, just reload dummy data to show responsiveness
    }
    m_controller->loadDummyData();
    m_statusBar->setPipelineState(false, m_controller->lastRunTime());
}

void MainWindow::onPipelineError(const QString& msg) {
    m_statusBar->setPipelineState(false, m_controller->lastRunTime());
    QMessageBox::warning(this, "Pipeline Error",
        "The ML pipeline encountered an error:\n\n" + msg);
}

// ─────────────────────────────────────────────
// Events
// ─────────────────────────────────────────────

void MainWindow::closeEvent(QCloseEvent* event) {
    if (m_worker && m_worker->isRunning()) {
        m_worker->requestStop();
        m_worker->wait(3000);
    }
    event->accept();
}

void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    // Dashboard panels must re-anchor on resize
    m_dashboardPage->updateGeometry();
    m_productsPage->updateGeometry();
}

} // namespace Kirana
