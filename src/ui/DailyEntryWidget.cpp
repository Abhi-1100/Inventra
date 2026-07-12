#include "ui/DailyEntryWidget.h"
#include "core/AppController.h"
#include "core/AuthController.h"
#include "core/Database.h"
#include "core/ProductData.h"
#include <QCompleter>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QCompleter>
#include <QSpinBox>
#include <QListWidget>
#include <QListWidgetItem>
#include <QTextEdit>
#include <QTabWidget>
#include <QFrame>
#include <QMessageBox>
#include <QInputDialog>
#include <QDate>
#include <QDateTime>

namespace Kirana {

DailyEntryWidget::DailyEntryWidget(AppController* controller,
                                    AuthController* auth,
                                    QWidget* parent)
    : QWidget(parent)
    , m_controller(controller)
    , m_auth(auth)
    , m_entryDate(QDate::currentDate())
{
    buildLayout();

    connect(m_controller, &AppController::productsChanged,
            this, &DailyEntryWidget::onProductsChanged);
    onProductsChanged();
    refreshList();
}

// ─────────────────────────────────────────────
// buildLayout
// ─────────────────────────────────────────────

void DailyEntryWidget::buildLayout() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 20, 24, 20);
    mainLayout->setSpacing(16);

    // ── Header ──────────────────────────────────
    auto* header = new QHBoxLayout;
    auto* title  = new QLabel(QStringLiteral("Daily Entry"));
    title->setStyleSheet(QStringLiteral(
        "font-size:18px; font-weight:600; color:#e6edf3; background:transparent;"));

    m_dateLabel = new QLabel(m_entryDate.toString(QStringLiteral("dddd, d MMMM yyyy")));
    m_dateLabel->setStyleSheet(QStringLiteral(
        "font-family:'JetBrains Mono','Consolas',monospace;"
        "font-size:12px; color:#6e7681; background:transparent;"));

    header->addWidget(title);
    header->addStretch();
    header->addWidget(m_dateLabel);
    mainLayout->addLayout(header);

    // ── Top section: tabs (form + bulk) + list side by side ──
    auto* contentRow = new QHBoxLayout;
    contentRow->setSpacing(16);

    // ── Left: Entry form tabs ──────────────────
    auto* leftPanel = new QFrame;
    leftPanel->setObjectName(QStringLiteral("PanelFrame"));
    leftPanel->setFixedWidth(420);
    auto* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(20, 20, 20, 20);
    leftLayout->setSpacing(12);

    m_tabs = new QTabWidget(leftPanel);
    m_tabs->setDocumentMode(true);

    // Tab 1: Single entry
    auto* singleTab = new QWidget;
    auto* singleLayout = new QVBoxLayout(singleTab);
    singleLayout->setContentsMargins(0, 12, 0, 0);
    singleLayout->setSpacing(10);

    auto makeFieldLabel = [](const QString& text) -> QLabel* {
        auto* l = new QLabel(text);
        l->setStyleSheet(QStringLiteral(
            "color:#6e7681; font-size:10px; font-weight:600;"
            "letter-spacing:0.5px; background:transparent;"));
        return l;
    };

    singleLayout->addWidget(makeFieldLabel(QStringLiteral("PRODUCT")));
    m_productCombo = new QComboBox(singleTab);
    m_productCombo->setEditable(true);
    m_productCombo->setInsertPolicy(QComboBox::NoInsert);
    m_productCombo->setMinimumHeight(38);
    m_productCombo->completer()->setCaseSensitivity(Qt::CaseInsensitive);
    singleLayout->addWidget(m_productCombo);

    auto* qtyRow = new QHBoxLayout;
    qtyRow->setSpacing(12);

    auto* soldCol = new QVBoxLayout;
    soldCol->addWidget(makeFieldLabel(QStringLiteral("UNITS SOLD")));
    m_soldSpin = new QSpinBox(singleTab);
    m_soldSpin->setRange(0, 99999);
    m_soldSpin->setMinimumHeight(38);
    m_soldSpin->setButtonSymbols(QAbstractSpinBox::PlusMinus);
    soldCol->addWidget(m_soldSpin);

    auto* wastedCol = new QVBoxLayout;
    wastedCol->addWidget(makeFieldLabel(QStringLiteral("WASTED / EXPIRED")));
    m_wastedSpin = new QSpinBox(singleTab);
    m_wastedSpin->setRange(0, 9999);
    m_wastedSpin->setMinimumHeight(38);
    m_wastedSpin->setButtonSymbols(QAbstractSpinBox::PlusMinus);
    m_wastedSpin->setStyleSheet(QStringLiteral(
        "QSpinBox { border-color: rgba(210,153,34,0.40); }"));
    wastedCol->addWidget(m_wastedSpin);

    qtyRow->addLayout(soldCol);
    qtyRow->addLayout(wastedCol);
    singleLayout->addLayout(qtyRow);

    m_addBtn = new QPushButton(QStringLiteral("+ Add Entry"));
    m_addBtn->setObjectName(QStringLiteral("PrimaryBtn"));
    m_addBtn->setMinimumHeight(40);
    singleLayout->addWidget(m_addBtn);
    singleLayout->addStretch();
    connect(m_addBtn, &QPushButton::clicked, this, &DailyEntryWidget::onAddEntryClicked);

    m_tabs->addTab(singleTab, QStringLiteral("Single Entry"));

    // Tab 2: Bulk paste
    auto* bulkTab = new QWidget;
    auto* bulkLayout = new QVBoxLayout(bulkTab);
    bulkLayout->setContentsMargins(0, 12, 0, 0);
    bulkLayout->setSpacing(10);

    auto* bulkHint = new QLabel(
        QStringLiteral("Paste one entry per line:\n  ProductName,UnitsSold\n  ProductName,UnitsSold,UnitsWasted"));
    bulkHint->setStyleSheet(QStringLiteral(
        "color:#6e7681; font-size:11px; background:#0e1318;"
        "border:1px solid #21262d; border-radius:4px; padding:8px;"));
    bulkHint->setWordWrap(true);
    bulkLayout->addWidget(bulkHint);

    m_bulkEdit = new QTextEdit(bulkTab);
    m_bulkEdit->setPlaceholderText(QStringLiteral(
        "Aashirvaad Atta 5kg, 12\nAmul Milk 1L, 24, 2\n..."));
    m_bulkEdit->setMinimumHeight(120);
    m_bulkEdit->setStyleSheet(QStringLiteral(
        "QTextEdit { font-family:'JetBrains Mono','Consolas',monospace; font-size:11px; }"));
    bulkLayout->addWidget(m_bulkEdit);

    m_bulkStatus = new QLabel;
    m_bulkStatus->setStyleSheet(QStringLiteral("color:#6e7681; font-size:10px; background:transparent;"));
    m_bulkStatus->setVisible(false);
    bulkLayout->addWidget(m_bulkStatus);

    m_bulkImport = new QPushButton(QStringLiteral("Parse & Import"));
    m_bulkImport->setObjectName(QStringLiteral("PrimaryBtn"));
    m_bulkImport->setMinimumHeight(40);
    bulkLayout->addWidget(m_bulkImport);
    bulkLayout->addStretch();
    connect(m_bulkImport, &QPushButton::clicked, this, &DailyEntryWidget::onBulkImportClicked);

    m_tabs->addTab(bulkTab, QStringLiteral("Bulk Paste"));
    leftLayout->addWidget(m_tabs);
    contentRow->addWidget(leftPanel);

    // ── Right: Today's entries list ────────────
    auto* rightPanel = new QFrame;
    rightPanel->setObjectName(QStringLiteral("PanelFrame"));
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(16, 16, 16, 16);
    rightLayout->setSpacing(10);

    auto* listHeader = new QHBoxLayout;
    auto* listTitle = new QLabel(QStringLiteral("TODAY'S ENTRIES"));
    listTitle->setStyleSheet(QStringLiteral(
        "color:#6e7681; font-size:10px; font-weight:600;"
        "letter-spacing:0.5px; background:transparent;"));

    m_summaryLabel = new QLabel;
    m_summaryLabel->setStyleSheet(QStringLiteral(
        "color:#8b949e; font-size:11px; background:transparent;"));

    listHeader->addWidget(listTitle);
    listHeader->addStretch();
    listHeader->addWidget(m_summaryLabel);
    rightLayout->addLayout(listHeader);

    m_entryList = new QListWidget(rightPanel);
    m_entryList->setAlternatingRowColors(true);
    m_entryList->setStyleSheet(QStringLiteral(
        "QListWidget::item { padding:10px 12px; }"));
    rightLayout->addWidget(m_entryList, 1);

    auto* listActions = new QHBoxLayout;
    m_editBtn = new QPushButton(QStringLiteral("Edit"));
    m_editBtn->setEnabled(false);
    m_deleteBtn = new QPushButton(QStringLiteral("Delete"));
    m_deleteBtn->setObjectName(QStringLiteral("DangerBtn"));
    m_deleteBtn->setEnabled(false);

    listActions->addStretch();
    listActions->addWidget(m_editBtn);
    listActions->addWidget(m_deleteBtn);
    rightLayout->addLayout(listActions);

    connect(m_entryList, &QListWidget::currentRowChanged, this, [this](int row) {
        m_editBtn->setEnabled(row >= 0);
        m_deleteBtn->setEnabled(row >= 0);
    });
    connect(m_deleteBtn, &QPushButton::clicked, this, &DailyEntryWidget::onDeleteEntryClicked);
    connect(m_editBtn,   &QPushButton::clicked, this, &DailyEntryWidget::onEditEntryClicked);

    contentRow->addWidget(rightPanel, 1);
    mainLayout->addLayout(contentRow, 1);
}

// ─────────────────────────────────────────────
// Product combo population
// ─────────────────────────────────────────────

void DailyEntryWidget::populateProductCombo() {
    const QString current = m_productCombo->currentText();
    m_productCombo->clear();
    for (const auto& p : m_controller->products()) {
        m_productCombo->addItem(p.name, p.id);
    }
    // Restore selection
    int idx = m_productCombo->findText(current);
    if (idx >= 0) m_productCombo->setCurrentIndex(idx);
}

void DailyEntryWidget::onProductsChanged() {
    populateProductCombo();
}

// ─────────────────────────────────────────────
// Add single entry
// ─────────────────────────────────────────────

void DailyEntryWidget::onAddEntryClicked() {
    if (m_productCombo->currentIndex() < 0 ||
        m_productCombo->currentText().trimmed().isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Daily Entry"),
            QStringLiteral("Please select a product."));
        return;
    }

    const int productId   = m_productCombo->currentData().toInt();
    const QString name    = m_productCombo->currentText();
    const int sold        = m_soldSpin->value();
    const int wasted      = m_wastedSpin->value();

    if (sold == 0 && wasted == 0) {
        QMessageBox::warning(this, QStringLiteral("Daily Entry"),
            QStringLiteral("Enter at least 1 unit sold or wasted."));
        return;
    }

    saveEntry(productId, name, sold, wasted);
    m_soldSpin->setValue(0);
    m_wastedSpin->setValue(0);
}

void DailyEntryWidget::saveEntry(int productId, const QString& productName,
                                  int sold, int wasted)
{
    auto* db = m_controller->database();
    if (!db) return;

    DailyEntry e;
    e.productId   = productId;
    e.productName = productName;
    e.entryDate   = m_entryDate;
    e.unitsSold   = sold;
    e.unitsWasted = wasted;
    e.enteredBy   = m_auth->currentUser().id;

    int id = db->saveDailyEntry(e);
    if (id < 0) {
        QMessageBox::warning(this, QStringLiteral("Error"),
            QStringLiteral("Failed to save entry: ") + db->lastError());
        return;
    }

    refreshList();
}

// ─────────────────────────────────────────────
// Bulk import
// ─────────────────────────────────────────────

void DailyEntryWidget::onBulkImportClicked() {
    const QString text = m_bulkEdit->toPlainText().trimmed();
    if (text.isEmpty()) return;

    const QStringList lines = text.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    int ok = 0, fail = 0;

    // Build product name→id map
    QHash<QString, int> nameToId;
    for (const auto& p : m_controller->products())
        nameToId.insert(p.name.toLower(), p.id);

    for (const QString& line : lines) {
        const QStringList parts = line.split(QLatin1Char(','));
        if (parts.size() < 2) { ++fail; continue; }

        const QString pName = parts[0].trimmed();
        bool okSold = false;
        int sold = parts[1].trimmed().toInt(&okSold);
        if (!okSold) { ++fail; continue; }

        int wasted = 0;
        if (parts.size() >= 3) wasted = parts[2].trimmed().toInt();

        int pid = nameToId.value(pName.toLower(), -1);
        if (pid < 0) { ++fail; continue; }

        saveEntry(pid, pName, sold, wasted);
        ++ok;
    }

    m_bulkStatus->setText(QStringLiteral("Imported %1 rows · %2 failed").arg(ok).arg(fail));
    m_bulkStatus->setStyleSheet(fail > 0
        ? QStringLiteral("color:#d29922; font-size:10px; background:transparent;")
        : QStringLiteral("color:#3fb950; font-size:10px; background:transparent;"));
    m_bulkStatus->setVisible(true);

    if (ok > 0) m_bulkEdit->clear();
}

// ─────────────────────────────────────────────
// Delete / Edit entry
// ─────────────────────────────────────────────

void DailyEntryWidget::onDeleteEntryClicked() {
    int row = m_entryList->currentRow();
    if (row < 0 || row >= m_entries.size()) return;

    const DailyEntry& e = m_entries[row];
    auto btn = QMessageBox::question(this, QStringLiteral("Delete Entry"),
        QStringLiteral("Delete entry for %1?").arg(e.productName),
        QMessageBox::Yes | QMessageBox::No);
    if (btn != QMessageBox::Yes) return;

    auto* db = m_controller->database();
    if (db) db->deleteDailyEntry(e.id);
    refreshList();
}

void DailyEntryWidget::onEditEntryClicked() {
    int row = m_entryList->currentRow();
    if (row < 0 || row >= m_entries.size()) return;

    DailyEntry e = m_entries[row];
    bool ok;
    int newSold = QInputDialog::getInt(this,
        QStringLiteral("Edit Units Sold"),
        QStringLiteral("Units sold for %1:").arg(e.productName),
        e.unitsSold, 0, 99999, 1, &ok);
    if (!ok) return;
    e.unitsSold = newSold;

    int newWasted = QInputDialog::getInt(this,
        QStringLiteral("Edit Units Wasted"),
        QStringLiteral("Units wasted/expired for %1:").arg(e.productName),
        e.unitsWasted, 0, 9999, 1, &ok);
    if (!ok) return;
    e.unitsWasted = newWasted;

    auto* db = m_controller->database();
    if (db) db->updateDailyEntry(e);
    refreshList();
}

// ─────────────────────────────────────────────
// refreshList — reload today's entries from DB
// ─────────────────────────────────────────────

void DailyEntryWidget::refreshList() {
    auto* db = m_controller->database();
    if (!db) return;

    m_entries = db->getDailyEntries(m_entryDate);
    m_entryList->clear();

    int totalSold = 0, totalWasted = 0;
    for (const auto& e : m_entries) {
        const QString text = QStringLiteral("%1  ·  sold: %2  wasted: %3")
            .arg(e.productName)
            .arg(e.unitsSold)
            .arg(e.unitsWasted);
        auto* item = new QListWidgetItem(text);
        item->setToolTip(e.createdAt.toString(QStringLiteral("hh:mm:ss")));
        m_entryList->addItem(item);
        totalSold   += e.unitsSold;
        totalWasted += e.unitsWasted;
    }

    m_summaryLabel->setText(
        QStringLiteral("%1 entries  ·  %2 sold  ·  %3 wasted")
        .arg(m_entries.size()).arg(totalSold).arg(totalWasted));
    m_deleteBtn->setEnabled(false);
    m_editBtn->setEnabled(false);
}

} // namespace Kirana
