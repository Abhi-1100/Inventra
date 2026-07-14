#include "ui/ProductCatalogWidget.h"
#include "core/AppController.h"
#include "core/ProductModel.h"
#include "core/ProductData.h"
#include "ui/ProductDetailPanel.h"
#include "widgets/BadgeDelegate.h"
#include "core/ThemeManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableView>
#include <QHeaderView>
#include <QSortFilterProxyModel>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QFrame>
#include <QResizeEvent>

namespace Kirana {

// ══════════════════════════════════════════════
// CatalogFilterProxy
// ══════════════════════════════════════════════

class CatalogFilterProxy : public QSortFilterProxyModel {
public:
    explicit CatalogFilterProxy(QObject* parent = nullptr)
        : QSortFilterProxyModel(parent)
    {}

    QString searchText;
    QString statusFilter;   // "" = all
    QString demandFilter;   // "" = all

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex&) const override {
        auto* m = qobject_cast<ProductModel*>(sourceModel());
        if (!m) return true;

        const Product& p = m->productAt(sourceRow);

        // Search text
        if (!searchText.isEmpty()) {
            if (!p.name.contains(searchText, Qt::CaseInsensitive) &&
                !p.sku.contains(searchText, Qt::CaseInsensitive))
                return false;
        }

        // Status filter
        if (!statusFilter.isEmpty()) {
            QString status;
            if      (p.priority == Priority::Critical)    status = "Critical";
            else if (p.priority == Priority::ReorderSoon) status = "Reorder";
            else if (p.stockStatus == StockStatus::Overstock) status = "Overstock";
            else status = "Safe";
            if (status != statusFilter) return false;
        }

        // Demand filter
        if (!demandFilter.isEmpty()) {
            if (toString(p.demandLabel) != demandFilter) return false;
        }

        return true;
    }
};

// ══════════════════════════════════════════════
// ProductCatalogWidget
// ══════════════════════════════════════════════

ProductCatalogWidget::ProductCatalogWidget(AppController* controller, QWidget* parent)
    : QWidget(parent)
    , m_controller(controller)
{
    buildLayout();
    configureTable();
    connect(m_controller, &AppController::productsChanged,
            this, &ProductCatalogWidget::onProductsChanged);
    onProductsChanged();
}

void ProductCatalogWidget::buildLayout() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 20, 24, 20);
    mainLayout->setSpacing(16);

    // ── Header ─────────────────────────────────
    auto* headerRow = new QHBoxLayout;
    headerRow->setSpacing(12);

    auto* pageTitle = new QLabel(QStringLiteral("Product Catalog"), this);
    pageTitle->setStyleSheet(
        "font-family:'SF Mono','Menlo','Cascadia Mono','Consolas',monospace;"
        "font-size:24px;font-weight:bold;color:#e5e5e5;"
        "background:transparent;border:none;");
    headerRow->addWidget(pageTitle);

    m_countLabel = new QLabel(QStringLiteral("0 products"), this);
    m_countLabel->setStyleSheet(
        "font-family:'SF Mono','Menlo','Cascadia Mono','Consolas',monospace;"
        "font-size:12px;color:#808080;"
        "background:rgba(217,119,6,0.08);"
        "border:1px solid #222222;"
        "border-radius:12px;padding:4px 12px;");
    m_countLabel->setAlignment(Qt::AlignCenter);
    headerRow->addWidget(m_countLabel);
    headerRow->addStretch();

    auto* addBtn = new QPushButton(QStringLiteral("+ Add Product"), this);
    addBtn->setObjectName("PrimaryBtn");
    addBtn->setFixedHeight(36);
    addBtn->setCursor(Qt::PointingHandCursor);
    addBtn->setStyleSheet(QString());
    headerRow->addWidget(addBtn);
    mainLayout->addLayout(headerRow);

    // ── Filter bar ──────────────────────────────
    const auto& tokens = ThemeManager::instance().tokens();
    auto* filterCard = new QFrame(this);
    filterCard->setObjectName("GlassCard");
    ThemeManager::applyDropShadow(filterCard, 20, QColor(tokens.Accent.red(), tokens.Accent.green(), tokens.Accent.blue(), 30));
    filterCard->setFixedHeight(64);
    filterCard->setStyleSheet(QString());

    auto* filterRow = new QHBoxLayout(filterCard);
    filterRow->setContentsMargins(16, 0, 16, 0);
    filterRow->setSpacing(12);

    // Search input
    auto* searchWrap = new QWidget(filterCard);
    searchWrap->setStyleSheet(
        "background:#050505;border:1px solid #222222;border-radius:3px;");
    searchWrap->setFixedWidth(260);
    auto* searchRow = new QHBoxLayout(searchWrap);
    searchRow->setContentsMargins(10, 0, 10, 0);
    searchRow->setSpacing(6);
    auto* searchIcon = new QLabel(QStringLiteral("⌕"), searchWrap);
    searchIcon->setStyleSheet("color:#808080;font-size:16px;background:transparent;border:none;");
    m_searchEdit = new QLineEdit(searchWrap);
    m_searchEdit->setPlaceholderText(QStringLiteral("Search products or SKU..."));
    m_searchEdit->setStyleSheet(
        "QLineEdit{background:transparent;border:none;"
        "color:#e5e5e5;font-family:'SF Mono','Menlo','Cascadia Mono','Consolas',monospace;font-size:13px;padding:0;}"
        "QLineEdit::placeholder{color:rgba(128,128,128,0.6);}");
    m_searchEdit->setFixedHeight(40);
    searchRow->addWidget(searchIcon);
    searchRow->addWidget(m_searchEdit);
    connect(m_searchEdit, &QLineEdit::textChanged,
            this, &ProductCatalogWidget::onSearchTextChanged);

    filterRow->addWidget(searchWrap);

    // Status filter
    auto makeFilter = [&](const QString& placeholder,
                           const QStringList& items) -> QComboBox* {
        auto* cb = new QComboBox(filterCard);
        cb->setFixedHeight(36);
        cb->setFixedWidth(160);
        cb->setCursor(Qt::PointingHandCursor);
        cb->addItem(placeholder);
        for (const auto& item : items) cb->addItem(item);
        cb->setStyleSheet(QString());
        return cb;
    };

    m_statusFilter = makeFilter(QStringLiteral("All Status"),
        { QStringLiteral("Critical"), QStringLiteral("Reorder"),
          QStringLiteral("Safe"),     QStringLiteral("Overstock") });
    m_demandFilter = makeFilter(QStringLiteral("All Demand"),
        { QStringLiteral("High"), QStringLiteral("Medium"), QStringLiteral("Low") });

    filterRow->addWidget(m_statusFilter);
    filterRow->addWidget(m_demandFilter);
    filterRow->addStretch();

    connect(m_statusFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ProductCatalogWidget::onFilterChanged);
    connect(m_demandFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ProductCatalogWidget::onFilterChanged);

    mainLayout->addWidget(filterCard);

    // ── Product table ────────────────────────────
    auto* tableCard = new QFrame(this);
    tableCard->setObjectName("GlassCard");
    ThemeManager::applyDropShadow(tableCard, 20, QColor(tokens.Accent.red(), tokens.Accent.green(), tokens.Accent.blue(), 30));
    tableCard->setStyleSheet(QString());

    auto* tableLayout = new QVBoxLayout(tableCard);
    tableLayout->setContentsMargins(0, 0, 0, 0);
    tableLayout->setSpacing(0);

    m_tableView = new QTableView(tableCard);
    m_tableView->setSortingEnabled(true);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->setShowGrid(false);
    m_tableView->setAlternatingRowColors(false);
    m_tableView->verticalHeader()->setVisible(false);
    m_tableView->verticalHeader()->setDefaultSectionSize(52);
    m_tableView->setFrameShape(QFrame::NoFrame);
    m_tableView->setStyleSheet("QTableView { border: none; background: transparent; }");

    tableLayout->addWidget(m_tableView);
    mainLayout->addWidget(tableCard, 1);

    // Detail panel
    m_detailPanel = new ProductDetailPanel(this);
    m_detailPanel->setVisible(false);
}

void ProductCatalogWidget::configureTable() {
    m_model = new ProductModel(this);

    auto* proxy = new CatalogFilterProxy(this);
    proxy->setSourceModel(m_model);
    proxy->setSortRole(Qt::DisplayRole);
    m_proxyModel = proxy;

    m_tableView->setModel(m_proxyModel);

    auto* badgeDelegate    = new BadgeDelegate(BadgeDelegate::Mode::Badge,    m_tableView);
    auto* forecastDelegate = new BadgeDelegate(BadgeDelegate::Mode::Forecast, m_tableView);

    m_tableView->setItemDelegateForColumn(static_cast<int>(ProductColumn::DemandLabel), badgeDelegate);
    m_tableView->setItemDelegateForColumn(static_cast<int>(ProductColumn::StockStatus), badgeDelegate);
    m_tableView->setItemDelegateForColumn(static_cast<int>(ProductColumn::Priority),    badgeDelegate);
    m_tableView->setItemDelegateForColumn(static_cast<int>(ProductColumn::Forecast),    forecastDelegate);

    QHeaderView* header = m_tableView->horizontalHeader();
    header->setSectionResizeMode(QHeaderView::Interactive);
    header->setStretchLastSection(true);
    header->setHighlightSections(false);
    header->setSortIndicatorShown(true);

    m_tableView->setColumnWidth(static_cast<int>(ProductColumn::Name),        240);
    m_tableView->setColumnWidth(static_cast<int>(ProductColumn::DemandLabel),  90);
    m_tableView->setColumnWidth(static_cast<int>(ProductColumn::StockStatus), 130);
    m_tableView->setColumnWidth(static_cast<int>(ProductColumn::Confidence),   90);
    m_tableView->setColumnWidth(static_cast<int>(ProductColumn::Forecast),    100);
    m_tableView->setColumnWidth(static_cast<int>(ProductColumn::Priority),    130);
    m_tableView->setColumnWidth(static_cast<int>(ProductColumn::EOQQty),       90);

    connect(m_tableView, &QTableView::doubleClicked,
            this, &ProductCatalogWidget::onRowDoubleClicked);
}

void ProductCatalogWidget::onProductsChanged() {
    m_model->setProducts(m_controller->products());
    m_countLabel->setText(
        QString::number(m_model->rowCount()) + QStringLiteral(" products"));
}

void ProductCatalogWidget::onRowDoubleClicked(const QModelIndex& index) {
    if (!index.isValid()) return;
    QModelIndex sourceIdx = m_proxyModel->mapToSource(index);
    const Product& product = m_model->productAt(sourceIdx.row());
    m_detailPanel->setProduct(product);
    m_detailPanel->slideIn();
}

void ProductCatalogWidget::onSearchTextChanged(const QString& text) {
    auto* proxy = static_cast<CatalogFilterProxy*>(m_proxyModel);
    if (proxy) { proxy->searchText = text; proxy->invalidate(); }
}

void ProductCatalogWidget::onFilterChanged() {
    auto* proxy = static_cast<CatalogFilterProxy*>(m_proxyModel);
    if (!proxy) return;

    proxy->statusFilter = (m_statusFilter->currentIndex() > 0)
                          ? m_statusFilter->currentText() : QString();
    proxy->demandFilter = (m_demandFilter->currentIndex() > 0)
                          ? m_demandFilter->currentText() : QString();
    proxy->invalidate();
}

void ProductCatalogWidget::refresh() {
    onProductsChanged();
}

void ProductCatalogWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    if (m_detailPanel && m_detailPanel->isVisible())
        m_detailPanel->adjustPanelPosition();
}

} // namespace Kirana
