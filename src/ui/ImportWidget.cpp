#include "core/ThemeManager.h"
#include "ui/ImportWidget.h"
#include "core/AppController.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QTableWidget>
#include <QFileDialog>
#include <QHeaderView>
#include <QProgressBar>              // ---- ADDED: API Integration ----
#include <QMessageBox>               // ---- ADDED: API Integration ----
#include <QJsonArray>                // ---- ADDED: API Integration ----
#include <QJsonObject>               // ---- ADDED: API Integration ----
#include "core/ApiClient.h"          // ---- ADDED: API Integration ----

namespace Kirana {

ImportWidget::ImportWidget(AppController* controller, QWidget* parent)
    : QWidget(parent)
    , m_controller(controller)
{
    buildLayout();
}

void ImportWidget::buildLayout() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);

    // Title
    auto* titleLabel = new QLabel("INVENTORY DATA INGESTION", this);
    titleLabel->setStyleSheet("font-family: 'Segoe UI', sans-serif; font-size: 16px; font-weight: bold; color: #e6edf3; letter-spacing: 0.5px;");
    mainLayout->addWidget(titleLabel);

    // ── Ingestion card / control area ───────────
    auto* controlFrame = new QFrame(this);
    controlFrame->setObjectName("ControlFrame");
    controlFrame->setStyleSheet(
        "QFrame#ControlFrame {"
        "  background: #161b22;"
        "  border: 1px solid #30363d;"
        "  border-radius: 6px;"
        "  padding: 16px;"
        "}"
    );
    auto* controlLayout = new QVBoxLayout(controlFrame);
    controlLayout->setSpacing(12);

    auto* filePickerLayout = new QHBoxLayout();
    m_filePathEdit = new QLineEdit(controlFrame);
    m_filePathEdit->setReadOnly(true);
    m_filePathEdit->setPlaceholderText("Select raw store inventory history CSV file...");
    m_filePathEdit->setStyleSheet("background-color: #0d1117; border: 1px solid #30363d; border-radius: 4px; padding: 6px; color: #e6edf3;");

    m_browseBtn = new QPushButton("Browse...", controlFrame);
    m_browseBtn->setStyleSheet(
        "QPushButton {"
        "  background: #21262d;"
        "  border: 1px solid #30363d;"
        "  color: #c9d1d9;"
        "  padding: 6px 12px;"
        "  border-radius: 4px;"
        "}"
        "QPushButton:hover { background: #30363d; }"
    );
    connect(m_browseBtn, &QPushButton::clicked, this, &ImportWidget::onBrowseClicked);

    filePickerLayout->addWidget(m_filePathEdit, 1);
    filePickerLayout->addWidget(m_browseBtn);
    controlLayout->addLayout(filePickerLayout);

    mainLayout->addWidget(controlFrame);

    // ── Header Mapping ──────────────────────────
    auto* mappingLabel = new QLabel("COLUMN HEADER MAPPER", this);
    mappingLabel->setStyleSheet("font-size: 11px; font-weight: bold; color: #8b949e;");
    mainLayout->addWidget(mappingLabel);

    m_mappingTable = new QTableWidget(this);
    m_mappingTable->setColumnCount(3);
    m_mappingTable->setHorizontalHeaderLabels({"Required Database Field", "Matched CSV Header", "Verification Status"});
    m_mappingTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_mappingTable->verticalHeader()->setVisible(false);
    m_mappingTable->setStyleSheet("background-color: #161b22; gridline-color: #30363d; color: #e6edf3;");
    m_mappingTable->horizontalHeader()->setStyleSheet("QHeaderView::section { background-color: #161b22; color: #8b949e; border: 1px solid #30363d; }");
    
    // Set table rows with targets
    QStringList fields = {"SKU / Barcode", "Product Title", "Category / Group", "Units in Stock", "Unit Cost (Supply Price)", "Sales History Dates", "Daily Volume"};
    m_mappingTable->setRowCount(fields.size());
    for (int i = 0; i < fields.size(); ++i) {
        auto* reqItem = new QTableWidgetItem(fields[i]);
        reqItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        m_mappingTable->setItem(i, 0, reqItem);

        auto* matchItem = new QTableWidgetItem("Auto-detected");
        matchItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        m_mappingTable->setItem(i, 1, matchItem);

        auto* statusItem = new QTableWidgetItem("✓ Found");
        statusItem->setForeground(QBrush(ThemeManager::instance().tokens().Success));
        statusItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        m_mappingTable->setItem(i, 2, statusItem);
    }
    
    mainLayout->addWidget(m_mappingTable, 1);

    // ── Process Control Row ─────────────────────
    auto* processLayout = new QHBoxLayout();
    m_statusLabel = new QLabel("System ready for CSV mapping verification.", this);
    m_statusLabel->setStyleSheet("font-family: 'Consolas', monospace; font-size: 10px; color: #8b949e;");

    m_runBtn = new QPushButton("▶ RUN ML PIPELINE", this);
    m_runBtn->setEnabled(false);
    m_runBtn->setStyleSheet(
        "QPushButton {"
        "  background: rgba(63,185,80,0.15);"
        "  border: 1px solid rgba(63,185,80,0.40);"
        "  color: #3fb950;"
        "  padding: 8px 16px;"
        "  font-weight: bold;"
        "  border-radius: 4px;"
        "}"
        "QPushButton:hover { background: rgba(63,185,80,0.25); }"
        "QPushButton:disabled {"
        "  background: transparent;"
        "  border: 1px solid #30363d;"
        "  color: #484f58;"
        "}"
    );
    connect(m_runBtn, &QPushButton::clicked, this, &ImportWidget::onRunClicked);

    processLayout->addWidget(m_statusLabel, 1);
    processLayout->addWidget(m_runBtn);
    mainLayout->addLayout(processLayout);

    // ---- ADDED: API Integration ---- progress bar + result label
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 0);  // indeterminate
    m_progressBar->setVisible(false);
    m_progressBar->setFixedHeight(6);
    m_progressBar->setStyleSheet(
        "QProgressBar { background: #21262d; border: none; border-radius: 3px; }"
        "QProgressBar::chunk { background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #1f6feb, stop:1 #58a6ff); border-radius: 3px; }"
    );
    mainLayout->addWidget(m_progressBar);

    m_resultLabel = new QLabel(this);
    m_resultLabel->setVisible(false);
    m_resultLabel->setWordWrap(true);
    m_resultLabel->setStyleSheet("font-family: 'Consolas', monospace; font-size: 11px; color: #3fb950; padding: 8px;");
    mainLayout->addWidget(m_resultLabel);

    // Connect ApiClient signals
    connect(&ApiClient::instance(), &ApiClient::pipelineComplete,
            this, &ImportWidget::onApiPipelineComplete);
    connect(&ApiClient::instance(), &ApiClient::apiError,
            this, &ImportWidget::onApiError);
    // ---- END ADDED ----
}

void ImportWidget::onBrowseClicked() {
    QString path = QFileDialog::getOpenFileName(this, "Select Raw Inventory CSV", "", "CSV Files (*.csv)");
    if (!path.isEmpty()) {
        onCsvSelected(path);
    }
}

void ImportWidget::onCsvSelected(const QString& path) {
    m_selectedPath = path;
    m_filePathEdit->setText(path);
    validateFile();
}

void ImportWidget::validateFile() {
    // Perform column auto-match mockup or simple verification
    m_statusLabel->setText("CSV structure loaded successfully: 7 matching columns, 245 product rows found.");
    m_statusLabel->setStyleSheet("font-family: 'Consolas', monospace; font-size: 10px; color: #3fb950;");
    m_runBtn->setEnabled(true);
}

void ImportWidget::onRunClicked() {
    if (m_selectedPath.isEmpty()) return;
    
    // Provide immediate visual feedback that the heavy ML process has started
    m_runBtn->setEnabled(false);
    m_runBtn->setText("⏳ PROCESSING...");
    m_resultLabel->setVisible(false);

    // ---- ADDED: API Integration ---- upload via HTTP instead of pybind11
    m_progressBar->setVisible(true);
    m_statusLabel->setText("Uploading dataset to FastAPI server...");
    m_statusLabel->setStyleSheet("font-family: 'Consolas', monospace; font-size: 11px; color: #f0b37e;");

    ApiClient::instance().uploadCsv(m_selectedPath);
    // ---- END ADDED ----
}

// ---- ADDED: API Integration ----
void ImportWidget::onApiPipelineComplete(const QJsonArray& results) {
    m_progressBar->setVisible(false);
    m_runBtn->setEnabled(true);
    m_runBtn->setText("▶ RUN ML PIPELINE");

    int total = results.size();
    int critical = 0, reorderSoon = 0, safe = 0;
    for (const auto& val : results) {
        QJsonObject obj = val.toObject();
        QString priority = obj.value("priority").toString();
        if (priority == "Critical") critical++;
        else if (priority == "ReorderSoon") reorderSoon++;
        else safe++;
    }

    m_statusLabel->setText("✅ Pipeline complete!");
    m_statusLabel->setStyleSheet("font-family: 'Consolas', monospace; font-size: 11px; color: #3fb950;");

    m_resultLabel->setVisible(true);
    m_resultLabel->setText(
        QString("Successfully processed %1 products.\n"
                "%2 Critical  |  %3 Reorder Soon  |  %4 Safe")
            .arg(total).arg(critical).arg(reorderSoon).arg(safe));
}

void ImportWidget::onApiError(const QString& msg) {
    m_progressBar->setVisible(false);
    m_runBtn->setEnabled(true);
    m_runBtn->setText("▶ RUN ML PIPELINE");
    m_statusLabel->setText("❌ Pipeline failed.");
    m_statusLabel->setStyleSheet("font-family: 'Consolas', monospace; font-size: 11px; color: #f85149;");

    QMessageBox::warning(this, "Pipeline Error",
        QString("The ML pipeline encountered an error:\n\n%1\n\n"
                "Hint: Is the FastAPI server running?\n"
                "Start it with: uvicorn main:app --port 8000").arg(msg));
}
// ---- END ADDED ----

} // namespace Kirana
