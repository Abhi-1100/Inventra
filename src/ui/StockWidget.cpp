#include "core/ThemeManager.h"
#include "ui/StockWidget.h"
#include "core/AppController.h"
#include "core/AuthController.h"
#include "core/Database.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QTabWidget>
#include <QLabel>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QDateEdit>
#include <QPushButton>
#include <QTableView>
#include <QStandardItemModel>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>

namespace Kirana {

StockWidget::StockWidget(AppController* controller,
                         AuthController* auth,
                         QWidget* parent)
    : QWidget(parent)
    , m_controller(controller)
    , m_auth(auth)
{
    buildLayout();
    populateProductCombos();
    loadLedger();

    connect(m_controller, &AppController::productsChanged, this, &StockWidget::onProductsChanged);
}

void StockWidget::buildLayout() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 20, 24, 20);
    mainLayout->setSpacing(16);

    // ── Header ──────────────────────────────────
    auto* header = new QHBoxLayout;
    auto* title = new QLabel(QStringLiteral("Stock Inventory Ledger"));
    title->setStyleSheet(QStringLiteral("font-size:18px; font-weight:600; color:#e6edf3; background:transparent;"));
    header->addWidget(title);
    header->addStretch();
    mainLayout->addLayout(header);

    // ── Top section: Form Tabs ─────────────────
    auto* tabWidget = new QTabWidget(this);
    tabWidget->setDocumentMode(true);

    auto makeFieldLabel = [](const QString& text) -> QLabel* {
        auto* l = new QLabel(text);
        l->setStyleSheet(QStringLiteral("color:#6e7681; font-size:10px; font-weight:600; letter-spacing:0.5px; background:transparent;"));
        return l;
    };

    // ── Tab 1: Stock In (Delivery/Restock) ───
    auto* stockInTab = new QWidget;
    auto* inGrid = new QGridLayout(stockInTab);
    inGrid->setContentsMargins(12, 12, 12, 12);
    inGrid->setSpacing(10);

    inGrid->addWidget(makeFieldLabel(QStringLiteral("PRODUCT")), 0, 0);
    m_inProductCombo = new QComboBox(stockInTab);
    m_inProductCombo->setEditable(true);
    m_inProductCombo->setInsertPolicy(QComboBox::NoInsert);
    m_inProductCombo->setMinimumHeight(38);
    m_inProductCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    inGrid->addWidget(m_inProductCombo, 1, 0);

    inGrid->addWidget(makeFieldLabel(QStringLiteral("QUANTITY")), 0, 1);
    m_inQtySpin = new QSpinBox(stockInTab);
    m_inQtySpin->setRange(1, 100000);
    m_inQtySpin->setMinimumHeight(38);
    inGrid->addWidget(m_inQtySpin, 1, 1);

    inGrid->addWidget(makeFieldLabel(QStringLiteral("SUPPLIER NAME")), 0, 2);
    m_inSupplierEdit = new QLineEdit(stockInTab);
    m_inSupplierEdit->setPlaceholderText(QStringLiteral("Supplier name"));
    m_inSupplierEdit->setMinimumHeight(38);
    inGrid->addWidget(m_inSupplierEdit, 1, 2);

    inGrid->addWidget(makeFieldLabel(QStringLiteral("COST PER UNIT")), 2, 0);
    m_inCostSpin = new QDoubleSpinBox(stockInTab);
    m_inCostSpin->setRange(0.0, 100000.0);
    m_inCostSpin->setValue(0.0);
    m_inCostSpin->setPrefix(QStringLiteral("₹ "));
    m_inCostSpin->setMinimumHeight(38);
    inGrid->addWidget(m_inCostSpin, 3, 0);

    inGrid->addWidget(makeFieldLabel(QStringLiteral("DELIVERY DATE")), 2, 1);
    m_inDateEdit = new QDateEdit(QDate::currentDate(), stockInTab);
    m_inDateEdit->setCalendarPopup(true);
    m_inDateEdit->setMinimumHeight(38);
    inGrid->addWidget(m_inDateEdit, 3, 1);

    auto* inSaveBtn = new QPushButton(QStringLiteral("↓ Record Restock (Stock In)"), stockInTab);
    inSaveBtn->setObjectName(QStringLiteral("PrimaryBtn"));
    inSaveBtn->setMinimumHeight(38);
    inGrid->addWidget(inSaveBtn, 3, 2);
    connect(inSaveBtn, &QPushButton::clicked, this, &StockWidget::onStockInSaved);

    tabWidget->addTab(stockInTab, QStringLiteral("Stock In (Restock)"));

    // ── Tab 2: Stock Out (Manual Removal) ────
    auto* stockOutTab = new QWidget;
    auto* outGrid = new QGridLayout(stockOutTab);
    outGrid->setContentsMargins(12, 12, 12, 12);
    outGrid->setSpacing(10);

    outGrid->addWidget(makeFieldLabel(QStringLiteral("PRODUCT")), 0, 0);
    m_outProductCombo = new QComboBox(stockOutTab);
    m_outProductCombo->setEditable(true);
    m_outProductCombo->setInsertPolicy(QComboBox::NoInsert);
    m_outProductCombo->setMinimumHeight(38);
    m_outProductCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    outGrid->addWidget(m_outProductCombo, 1, 0);

    outGrid->addWidget(makeFieldLabel(QStringLiteral("QUANTITY")), 0, 1);
    m_outQtySpin = new QSpinBox(stockOutTab);
    m_outQtySpin->setRange(1, 100000);
    m_outQtySpin->setMinimumHeight(38);
    outGrid->addWidget(m_outQtySpin, 1, 1);

    outGrid->addWidget(makeFieldLabel(QStringLiteral("REASON")), 0, 2);
    m_outReasonCombo = new QComboBox(stockOutTab);
    m_outReasonCombo->addItem(QStringLiteral("Damage"));
    m_outReasonCombo->addItem(QStringLiteral("Theft"));
    m_outReasonCombo->addItem(QStringLiteral("Return to Supplier"));
    m_outReasonCombo->addItem(QStringLiteral("Expiry"));
    m_outReasonCombo->addItem(QStringLiteral("Other"));
    m_outReasonCombo->setMinimumHeight(38);
    outGrid->addWidget(m_outReasonCombo, 1, 2);

    outGrid->addWidget(makeFieldLabel(QStringLiteral("REMOVAL DATE")), 2, 0);
    m_outDateEdit = new QDateEdit(QDate::currentDate(), stockOutTab);
    m_outDateEdit->setCalendarPopup(true);
    m_outDateEdit->setMinimumHeight(38);
    outGrid->addWidget(m_outDateEdit, 3, 0);

    auto* outSaveBtn = new QPushButton(QStringLiteral("↑ Record Removal (Stock Out)"), stockOutTab);
    outSaveBtn->setObjectName(QStringLiteral("DangerBtn"));
    outSaveBtn->setMinimumHeight(38);
    outGrid->addWidget(outSaveBtn, 3, 2);
    connect(outSaveBtn, &QPushButton::clicked, this, &StockWidget::onStockOutSaved);

    tabWidget->addTab(stockOutTab, QStringLiteral("Stock Out (Manual Removal)"));

    mainLayout->addWidget(tabWidget);

    // ── Bottom section: Ledger Table & Filters ──
    auto* ledgerFrame = new QFrame(this);
    ledgerFrame->setObjectName(QStringLiteral("PanelFrame"));
    ThemeManager::applyDropShadow(ledgerFrame, 20, QColor(31, 111, 235, 30));
    auto* ledgerLayout = new QVBoxLayout(ledgerFrame);
    ledgerLayout->setContentsMargins(16, 16, 16, 16);
    ledgerLayout->setSpacing(12);

    auto* filterRow = new QHBoxLayout;
    filterRow->setSpacing(12);

    filterRow->addWidget(new QLabel(QStringLiteral("From:")));
    m_filterFromDate = new QDateEdit(QDate::currentDate().addDays(-30), ledgerFrame);
    m_filterFromDate->setCalendarPopup(true);
    connect(m_filterFromDate, &QDateEdit::dateChanged, this, &StockWidget::onFilterChanged);
    filterRow->addWidget(m_filterFromDate);

    filterRow->addWidget(new QLabel(QStringLiteral("To:")));
    m_filterToDate = new QDateEdit(QDate::currentDate(), ledgerFrame);
    m_filterToDate->setCalendarPopup(true);
    connect(m_filterToDate, &QDateEdit::dateChanged, this, &StockWidget::onFilterChanged);
    filterRow->addWidget(m_filterToDate);

    filterRow->addWidget(new QLabel(QStringLiteral("Type:")));
    m_filterTypeCombo = new QComboBox(ledgerFrame);
    m_filterTypeCombo->addItem(QStringLiteral("All Movements"), QStringLiteral(""));
    m_filterTypeCombo->addItem(QStringLiteral("Stock In (Restocks)"), QStringLiteral("IN"));
    m_filterTypeCombo->addItem(QStringLiteral("Stock Out (Removals)"), QStringLiteral("OUT"));
    connect(m_filterTypeCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &StockWidget::onFilterChanged);
    filterRow->addWidget(m_filterTypeCombo);

    filterRow->addStretch();

    auto* exportBtn = new QPushButton(QStringLiteral("Export CSV"));
    connect(exportBtn, &QPushButton::clicked, this, &StockWidget::onExportCsv);
    filterRow->addWidget(exportBtn);

    ledgerLayout->addLayout(filterRow);

    // Table Setup
    m_ledgerTable = new QTableView(ledgerFrame);
    m_ledgerTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_ledgerTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_ledgerTable->setAlternatingRowColors(true);
    m_ledgerTable->verticalHeader()->setVisible(false);
    m_ledgerTable->horizontalHeader()->setStretchLastSection(true);

    m_ledgerModel = new QStandardItemModel(this);
    m_ledgerModel->setHorizontalHeaderLabels({
        QStringLiteral("DATE"), QStringLiteral("PRODUCT"), QStringLiteral("TYPE"),
        QStringLiteral("QTY"), QStringLiteral("COST/UNIT"), QStringLiteral("SUPPLIER / REASON"),
        QStringLiteral("RECORDED BY")
    });
    m_ledgerTable->setModel(m_ledgerModel);

    ledgerLayout->addWidget(m_ledgerTable, 1);
    mainLayout->addWidget(ledgerFrame, 1);
}

void StockWidget::populateProductCombos() {
    QString currentIn = m_inProductCombo->currentText();
    QString currentOut = m_outProductCombo->currentText();

    m_inProductCombo->clear();
    m_outProductCombo->clear();

    for (const auto& p : m_controller->products()) {
        m_inProductCombo->addItem(p.name, p.id);
        m_outProductCombo->addItem(p.name, p.id);
    }

    int idxIn = m_inProductCombo->findText(currentIn);
    if (idxIn >= 0) m_inProductCombo->setCurrentIndex(idxIn);

    int idxOut = m_outProductCombo->findText(currentOut);
    if (idxOut >= 0) m_outProductCombo->setCurrentIndex(idxOut);
}

void StockWidget::onProductsChanged() {
    populateProductCombos();
}

void StockWidget::onStockInSaved() {
    if (m_inProductCombo->currentIndex() < 0 || m_inProductCombo->currentText().trimmed().isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Restock"), QStringLiteral("Select a product to restock."));
        return;
    }

    auto* db = m_controller->database();
    if (!db) return;

    StockMovement mv;
    mv.productId = m_inProductCombo->currentData().toInt();
    mv.movementType = QStringLiteral("IN");
    mv.quantity = m_inQtySpin->value();
    mv.supplierName = m_inSupplierEdit->text().trimmed();
    mv.costPerUnit = m_inCostSpin->value();
    mv.movementDate = m_inDateEdit->date();
    mv.enteredBy = m_auth->currentUser().id;

    if (db->saveStockMovement(mv) > 0) {
        // Reset Form
        m_inQtySpin->setValue(1);
        m_inSupplierEdit->clear();
        m_inCostSpin->setValue(0.0);
        // Refresh
        loadLedger();
        m_controller->loadFromDatabase(); // reload controller products to update cache & UI
    } else {
        QMessageBox::critical(this, QStringLiteral("Error"), QStringLiteral("Could not save restock entry."));
    }
}

void StockWidget::onStockOutSaved() {
    if (m_outProductCombo->currentIndex() < 0 || m_outProductCombo->currentText().trimmed().isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Stock Out"), QStringLiteral("Select a product to adjust/remove."));
        return;
    }

    auto* db = m_controller->database();
    if (!db) return;

    StockMovement mv;
    mv.productId = m_outProductCombo->currentData().toInt();
    mv.movementType = QStringLiteral("OUT");
    mv.quantity = m_outQtySpin->value();
    mv.reason = m_outReasonCombo->currentText();
    mv.movementDate = m_outDateEdit->date();
    mv.enteredBy = m_auth->currentUser().id;

    if (db->saveStockMovement(mv) > 0) {
        // Reset Form
        m_outQtySpin->setValue(1);
        // Refresh
        loadLedger();
        m_controller->loadFromDatabase(); // reload controller products
    } else {
        QMessageBox::critical(this, QStringLiteral("Error"), QStringLiteral("Could not save stock adjustment entry."));
    }
}

void StockWidget::loadLedger() {
    auto* db = m_controller->database();
    if (!db) return;

    m_ledgerModel->removeRows(0, m_ledgerModel->rowCount());

    QDate from = m_filterFromDate->date();
    QDate to = m_filterToDate->date();
    QString type = m_filterTypeCombo->currentData().toString();

    QVector<StockMovement> movements = db->getStockMovements(from, to, type);

    for (const auto& mv : movements) {
        QList<QStandardItem*> row;
        row.append(new QStandardItem(mv.movementDate.toString(Qt::ISODate)));
        row.append(new QStandardItem(mv.productName));
        row.append(new QStandardItem(mv.movementType));
        row.append(new QStandardItem(QString::number(mv.quantity)));
        row.append(new QStandardItem(mv.movementType == QLatin1String("IN") ? QStringLiteral("₹%1").arg(mv.costPerUnit, 0, 'f', 2) : QStringLiteral("—")));
        row.append(new QStandardItem(mv.movementType == QLatin1String("IN") ? mv.supplierName : mv.reason));
        row.append(new QStandardItem(mv.staffName.isEmpty() ? QStringLiteral("Owner") : mv.staffName));

        // Styling
        if (mv.movementType == QLatin1String("IN")) {
            row[2]->setForeground(ThemeManager::instance().tokens().Success);
        } else {
            row[2]->setForeground(ThemeManager::instance().tokens().Critical);
        }

        m_ledgerModel->appendRow(row);
    }

    // Set styling of table columns
    m_ledgerTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_ledgerTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_ledgerTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_ledgerTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_ledgerTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_ledgerTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Stretch);
    m_ledgerTable->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
}

void StockWidget::onFilterChanged() {
    loadLedger();
}

void StockWidget::onExportCsv() {
    QString path = QFileDialog::getSaveFileName(this, QStringLiteral("Export Ledger"), {}, QStringLiteral("CSV Files (*.csv)"));
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, QStringLiteral("Export Error"), QStringLiteral("Could not open file for writing."));
        return;
    }

    QTextStream out(&file);
    // Write headers
    out << "DATE,PRODUCT,TYPE,QTY,COST/UNIT,SUPPLIER/REASON,RECORDED BY\n";

    for (int r = 0; r < m_ledgerModel->rowCount(); ++r) {
        QStringList cells;
        for (int c = 0; c < m_ledgerModel->columnCount(); ++c) {
            QString txt = m_ledgerModel->item(r, c)->text();
            // Wrap in quotes if comma is present
            if (txt.contains(QLatin1Char(','))) {
                txt = QStringLiteral("\"%1\"").arg(txt);
            }
            cells.append(txt);
        }
        out << cells.join(QLatin1Char(',')) << "\n";
    }

    QMessageBox::information(this, QStringLiteral("Export"), QStringLiteral("Ledger exported successfully."));
}

} // namespace Kirana
