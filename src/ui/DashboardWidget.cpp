#include "ui/DashboardWidget.h"
#include "core/AppController.h"
#include "core/ProductModel.h"
#include "widgets/MetricCard.h"
#include "widgets/BadgeDelegate.h"
#include "ui/ProductDetailPanel.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableView>
#include <QHeaderView>
#include <QSortFilterProxyModel>
#include <QResizeEvent>

namespace Kirana {

DashboardWidget::DashboardWidget(AppController* controller, QWidget* parent)
    : QWidget(parent)
    , m_controller(controller)
{
    buildLayout();
    configureTable();

    connect(m_controller, &AppController::productsChanged, this, &DashboardWidget::onProductsChanged);
    
    onProductsChanged(); // initial load
}

void DashboardWidget::buildLayout() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);

    // ── Metric Cards Row ──────────────────────────
    auto* cardsLayout = new QHBoxLayout();
    cardsLayout->setSpacing(16);

    m_cardSKUs = new MetricCard("Total SKUs", QColor(Palette::Info), this);
    m_cardCritical = new MetricCard("Critical Reorder", QColor(Palette::Critical), this);
    m_cardOverstock = new MetricCard("Overstock", QColor(Palette::Warning), this);
    m_cardStockout = new MetricCard("Stockout Risk %", QColor(Palette::Success), this);

    cardsLayout->addWidget(m_cardSKUs);
    cardsLayout->addWidget(m_cardCritical);
    cardsLayout->addWidget(m_cardOverstock);
    cardsLayout->addWidget(m_cardStockout);

    mainLayout->addLayout(cardsLayout);

    // ── Table Container ───────────────────────────
    m_tableView = new QTableView(this);
    m_tableView->setObjectName("ProductTableView");
    m_tableView->setSortingEnabled(true);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->setShowGrid(true);
    m_tableView->setGridStyle(Qt::SolidLine);
    m_tableView->setAlternatingRowColors(true);
    
    // Set custom horizontal/vertical headers styling in code or stylesheet
    m_tableView->verticalHeader()->setVisible(false);
    m_tableView->verticalHeader()->setDefaultSectionSize(42);
    
    mainLayout->addWidget(m_tableView, 1);

    // ── Detail Panel (hidden slide-out overlay) ───
    m_detailPanel = new ProductDetailPanel(this);
    m_detailPanel->setVisible(false);
}

void DashboardWidget::configureTable() {
    m_model = new ProductModel(this);
    
    m_proxyModel = new QSortFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_model);
    m_proxyModel->setSortRole(Qt::DisplayRole); // Default sorting role
    
    m_tableView->setModel(m_proxyModel);

    // Set custom delegates for Badges and Forecast
    auto* badgeDelegate = new BadgeDelegate(BadgeDelegate::Mode::Badge, m_tableView);
    auto* forecastDelegate = new BadgeDelegate(BadgeDelegate::Mode::Forecast, m_tableView);

    m_tableView->setItemDelegateForColumn(static_cast<int>(ProductColumn::DemandLabel), badgeDelegate);
    m_tableView->setItemDelegateForColumn(static_cast<int>(ProductColumn::StockStatus), badgeDelegate);
    m_tableView->setItemDelegateForColumn(static_cast<int>(ProductColumn::Priority), badgeDelegate);
    m_tableView->setItemDelegateForColumn(static_cast<int>(ProductColumn::Forecast), forecastDelegate);

    // Table view header styling
    QHeaderView* header = m_tableView->horizontalHeader();
    header->setSectionResizeMode(QHeaderView::Interactive);
    header->setStretchLastSection(true);
    
    // Set column widths
    m_tableView->setColumnWidth(static_cast<int>(ProductColumn::Name), 280);
    m_tableView->setColumnWidth(static_cast<int>(ProductColumn::DemandLabel), 110);
    m_tableView->setColumnWidth(static_cast<int>(ProductColumn::StockStatus), 120);
    m_tableView->setColumnWidth(static_cast<int>(ProductColumn::Confidence), 110);
    m_tableView->setColumnWidth(static_cast<int>(ProductColumn::Forecast), 140);
    m_tableView->setColumnWidth(static_cast<int>(ProductColumn::Priority), 130);
    m_tableView->setColumnWidth(static_cast<int>(ProductColumn::EOQQty), 100);

    connect(m_tableView, &QTableView::doubleClicked, this, &DashboardWidget::onRowDoubleClicked);
}

void DashboardWidget::updateMetrics() {
    m_cardSKUs->setValue(QString::number(m_controller->products().size()));
    m_cardSKUs->setSubtitle("Total active database items");

    m_cardCritical->setValue(QString::number(m_controller->criticalCount()));
    m_cardCritical->setSubtitle("Need immediate stock order");

    m_cardOverstock->setValue(QString::number(m_controller->overstockCount()));
    m_cardOverstock->setSubtitle("Excess capital locked in stock");

    m_cardStockout->setValue(QString("%1%").arg(m_controller->stockoutRiskPct(), 0, 'f', 1));
    m_cardStockout->setSubtitle("Products at high stockout risk");
}

void DashboardWidget::onProductsChanged() {
    m_model->setProducts(m_controller->products());
    updateMetrics();
}

void DashboardWidget::onRowDoubleClicked(const QModelIndex& index) {
    if (!index.isValid()) return;
    
    // Get product from proxy model mapping
    QModelIndex sourceIdx = m_proxyModel->mapToSource(index);
    const Product& product = m_model->productAt(sourceIdx.row());
    
    m_detailPanel->setProduct(product);
    m_detailPanel->slideIn();
}

void DashboardWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    if (m_detailPanel && m_detailPanel->isVisible()) {
        m_detailPanel->adjustPanelPosition();
    }
}

} // namespace Kirana
