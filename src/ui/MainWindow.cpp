#include "ui/MainWindow.h"
#include "ui/Sidebar.h"
#include "ui/StatusBar.h"
#include "ui/AuthWidget.h"
#include "ui/DashboardWidget.h"
#include "ui/DailyEntryWidget.h"
#include "ui/StockWidget.h"
#include "ui/AnalyticsWidget.h"
#include "ui/ImportWidget.h"
#include "ui/SettingsWidget.h"
#include "core/AppController.h"
#include "core/AuthController.h"
#include "core/PipelineWorker.h"
#include "python/PipelineResult.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QCloseEvent>
#include <QResizeEvent>
#include <QMessageBox>
#include <QStatusBar>

namespace Kirana {

MainWindow::MainWindow(AppController* controller,
                       AuthController* auth,
                       QWidget* parent)
    : QMainWindow(parent)
    , m_controller(controller)
    , m_auth(auth)
{
    setObjectName(QStringLiteral("MainWindow"));
    setWindowTitle(QStringLiteral("Inventra  v1.0"));
    setMinimumSize(1280, 800);

    // Qt status bar hidden — we use our own
    statusBar()->hide();

    // Root: auth vs. app-shell stacked widget
    m_rootStack = new QStackedWidget(this);
    setCentralWidget(m_rootStack);

    buildAuthLayer();

    // Connect auth signals before showing anything
    connectSignals();

    // Determine initial state
    if (m_auth->isRegistered()) {
        // Show Login screen
        m_authWidget->setMode(AuthWidget::Mode::Login);
        m_rootStack->setCurrentIndex(0);
    } else {
        // First launch: show Registration
        m_authWidget->setMode(AuthWidget::Mode::Registration);
        m_rootStack->setCurrentIndex(0);
    }
}

MainWindow::~MainWindow() {
    if (m_worker && m_worker->isRunning()) {
        m_worker->requestStop();
        m_worker->wait(3000);
    }
}

// ─────────────────────────────────────────────
// buildAuthLayer
// ─────────────────────────────────────────────

void MainWindow::buildAuthLayer() {
    m_authWidget = new AuthWidget(m_auth, this);
    m_rootStack->addWidget(m_authWidget);   // index 0
}

// ─────────────────────────────────────────────
// buildAppShell  (called on first login)
// ─────────────────────────────────────────────

void MainWindow::buildAppShell() {
    if (m_appShell) return;  // already built

    m_appShell = new QWidget(this);
    m_appShell->setObjectName(QStringLiteral("AppShell"));

    auto* rootLayout = new QHBoxLayout(m_appShell);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // ── Left: Sidebar ─────────────────────────
    m_sidebar = new Sidebar(m_appShell);
    rootLayout->addWidget(m_sidebar);

    // ── Right: body ───────────────────────────
    auto* body = new QWidget(m_appShell);
    body->setObjectName(QStringLiteral("Body"));
    auto* bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(0);

    m_statusBar = new StatusBar(body);
    bodyLayout->addWidget(m_statusBar);

    // Page stack — indices match Sidebar::Page enum
    m_stack = new QStackedWidget(body);
    m_stack->setObjectName(QStringLiteral("PageStack"));

    m_dashboardPage  = new DashboardWidget(m_controller, body);
    m_dailyEntryPage = new DailyEntryWidget(m_controller, m_auth, body);
    m_stockPage      = new StockWidget(m_controller, m_auth, body);
    m_productsPage   = new DashboardWidget(m_controller, body);
    m_analyticsPage  = new AnalyticsWidget(m_controller, body);
    m_importPage     = new ImportWidget(m_controller, body);
    m_settingsPage   = new SettingsWidget(m_controller, m_auth, body);

    m_stack->addWidget(m_dashboardPage);    // 0: Dashboard
    m_stack->addWidget(m_dailyEntryPage);   // 1: Daily Entry
    m_stack->addWidget(m_stockPage);        // 2: Stock In/Out
    m_stack->addWidget(m_productsPage);     // 3: Products
    m_stack->addWidget(m_analyticsPage);    // 4: Analytics
    m_stack->addWidget(m_importPage);       // 5: Import
    m_stack->addWidget(m_settingsPage);     // 6: Settings

    bodyLayout->addWidget(m_stack, 1);
    rootLayout->addWidget(body, 1);

    m_worker = new PipelineWorker(m_appShell);

    m_rootStack->addWidget(m_appShell);   // index 1

    // Connect shell signals
    connect(m_sidebar, &Sidebar::pageSelected,
            this, &MainWindow::onPageSelected);

    connect(m_statusBar, &StatusBar::runNowRequested,
            this, &MainWindow::onRunNowClicked);

    connect(m_importPage, &ImportWidget::pipelineRunRequested,
            this, [this](const QString& csvPath) {
                m_worker->setCsvPath(csvPath);
                m_worker->setSettings(m_controller->settings());
                if (!m_worker->isRunning()) m_worker->start();
            });

    connect(m_settingsPage, &SettingsWidget::settingsChanged,
            m_controller, &AppController::updateSettings);
    connect(m_settingsPage, &SettingsWidget::reRunRequested,
            this, &MainWindow::onRunNowClicked);

    connect(m_worker, &PipelineWorker::pipelineStarted,
            this, &MainWindow::onPipelineStarted, Qt::QueuedConnection);
    connect(m_worker, &PipelineWorker::progressUpdate,
            this, &MainWindow::onPipelineProgress, Qt::QueuedConnection);
    connect(m_worker, &PipelineWorker::pipelineFinished,
            this, &MainWindow::onPipelineFinished, Qt::QueuedConnection);
    connect(m_worker, &PipelineWorker::pipelineError,
            this, &MainWindow::onPipelineError, Qt::QueuedConnection);

    connect(m_controller, &AppController::pipelineStateChanged,
            m_statusBar, &StatusBar::setPipelineState);
}

// ─────────────────────────────────────────────
// connectSignals  (auth signals, connected early)
// ─────────────────────────────────────────────

void MainWindow::connectSignals() {
    connect(m_auth, &AuthController::loginSucceeded,
            this, &MainWindow::onLoginSucceeded);
    connect(m_auth, &AuthController::loginFailed,
            m_authWidget, &AuthWidget::onLoginFailed);
    connect(m_auth, &AuthController::loggedOut,
            this, &MainWindow::onLoggedOut);
}

// ─────────────────────────────────────────────
// showAppShell  (public, called externally too)
// ─────────────────────────────────────────────

void MainWindow::showAppShell() {
    buildAppShell();
    m_rootStack->setCurrentIndex(1);
    onPageSelected(Sidebar::Page::Dashboard);
}

// ─────────────────────────────────────────────
// Slots
// ─────────────────────────────────────────────

void MainWindow::onLoginSucceeded(const StaffUser& user) {
    buildAppShell();
    // Apply role gating
    m_sidebar->setRole(user.role);
    m_rootStack->setCurrentIndex(1);
    showPage(static_cast<int>(Sidebar::Page::Dashboard));
}

void MainWindow::onLoggedOut() {
    m_rootStack->setCurrentIndex(0);
    m_authWidget->setMode(AuthWidget::Mode::Login);
}

void MainWindow::onPageSelected(Sidebar::Page page) {
    showPage(static_cast<int>(page));
}

void MainWindow::showPage(int index) {
    if (!m_stack) return;
    m_stack->setCurrentIndex(index);
    m_sidebar->setActivePage(static_cast<Sidebar::Page>(index));
}

void MainWindow::onRunNowClicked() {
    if (!m_worker || m_worker->isRunning()) return;
    m_worker->setCsvPath(QString());
    m_worker->setSettings(m_controller->settings());
    m_worker->start();
}

void MainWindow::onPipelineStarted() {
    m_statusBar->setPipelineState(true, m_controller->lastRunTime());
}

void MainWindow::onPipelineProgress(int /*pct*/, const QString& /*stage*/) {}

void MainWindow::onPipelineFinished(const PipelineRunResult& result) {
    if (result.success && !result.results.isEmpty()) {
        // Phase 2: applyPipelineResults
    }
    m_controller->loadDummyData();
    m_statusBar->setPipelineState(false, m_controller->lastRunTime());
}

void MainWindow::onPipelineError(const QString& msg) {
    m_statusBar->setPipelineState(false, m_controller->lastRunTime());
    QMessageBox::warning(this, QStringLiteral("Pipeline Error"),
        QStringLiteral("The ML pipeline encountered an error:\n\n") + msg);
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
    if (m_dashboardPage) m_dashboardPage->updateGeometry();
    if (m_productsPage)  m_productsPage->updateGeometry();
}

} // namespace Kirana
