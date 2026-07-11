#include "ui/DashboardWidget.h"
#include "core/AppController.h"
#include "core/Database.h"
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
#include <QPushButton>
#include <QListWidget>
#include <QFrame>
#include <QLabel>

namespace Kirana {

// ─────────────────────────────────────────────
// ProductFilterProxyModel
// Elegant internal subclass for dashboard filter chips
// ─────────────────────────────────────────────

class ProductFilterProxyModel : public QSortFilterProxyModel {
public:
    explicit ProductFilterProxyModel(QObject* parent = nullptr) 
        : QSortFilterProxyModel(parent) 
    {}

    QString filterState = QStringLiteral("All");

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override {
        if (filterState.isEmpty() || filterState == QLatin1String("All")) {
            return true;
        }

        auto* model = qobject_cast<ProductModel*>(sourceModel());
        if (!model) return true;

        const Product& p = model->productAt(sourceRow);

        if (filterState == QLatin1String("Critical")) {
            return p.priority == Priority::Critical;
        } else if (filterState == QLatin1String("Reorder")) {
            return p.stockStatus == StockStatus::Reorder || p.priority == Priority::ReorderSoon;
        } else if (filterState == QLatin1String("Overstock")) {
            return p.stockStatus == StockStatus::Overstock;
        } else if (filterState == QLatin1String("Safe")) {
            return p.priority == Priority::Safe;
        }

        return true;
    }
};

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

    m_cardSKUs = new MetricCard(QStringLiteral("Total SKUs"), QColor(Palette::Info), this);
    m_cardCritical = new MetricCard(QStringLiteral("Critical Reorder"), QColor(Palette::Critical), this);
    m_cardOverstock = new MetricCard(QStringLiteral("Overstock"), QColor(Palette::Warning), this);
    m_cardStockout = new MetricCard(QStringLiteral("Stockout Risk %"), QColor(Palette::Success), this);

    cardsLayout->addWidget(m_cardSKUs);
    cardsLayout->addWidget(m_cardCritical);
    cardsLayout->addWidget(m_cardOverstock);
    cardsLayout->addWidget(m_cardStockout);

    mainLayout->addLayout(cardsLayout);

    // ── Horizontal Layout for Table (Left) & Activity Feed (Right) ──
    auto* contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(20);

    // Left Container (Filter chips row + Table view)
    auto* leftContainer = new QVBoxLayout();
    leftContainer->setSpacing(12);

    // Filter Chips row
    auto* chipsRow = new QHBoxLayout();
    chipsRow->setSpacing(8);
    setupFilterChips(chipsRow);
    leftContainer->addLayout(chipsRow);

    // Table view
    m_tableView = new QTableView(this);
    m_tableView->setObjectName(QStringLiteral("ProductTableView"));
    m_tableView->setSortingEnabled(true);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->setShowGrid(true);
    m_tableView->setGridStyle(Qt::SolidLine);
    m_tableView->setAlternatingRowColors(true);
    m_tableView->verticalHeader()->setVisible(false);
    m_tableView->verticalHeader()->setDefaultSectionSize(42);

    leftContainer->addWidget(m_tableView, 1);
    contentLayout->addLayout(leftContainer, 3); // 3x weight

    // Right Container: Activity Feed
    auto* rightPanel = new QFrame(this);
    rightPanel->setObjectName(QStringLiteral("ActivityFeed"));
    rightPanel->setFixedWidth(280);
    
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(16, 16, 16, 16);
    rightLayout->setSpacing(12);

    auto* feedTitle = new QLabel(QStringLiteral("RECENT ACTIVITY FEED"), rightPanel);
    feedTitle->setObjectName(QStringLiteral("ActivityFeedHeader"));
    rightLayout->addWidget(feedTitle);

    m_activityList = new QListWidget(rightPanel);
    m_activityList->setStyleSheet(QStringLiteral("QListWidget { background: transparent; border: none; }"));
    rightLayout->addWidget(m_activityList, 1);

    contentLayout->addWidget(rightPanel, 1); // 1x weight
    mainLayout->addLayout(contentLayout, 1);

    // ── Detail Panel (hidden slide-out overlay) ───
    m_detailPanel = new ProductDetailPanel(this);
    m_detailPanel->setVisible(false);
}

void DashboardWidget::setupFilterChips(QHBoxLayout* rowLayout) {
    struct ChipInfo {
        QString label;
        QString value;
    };
    QVector<ChipInfo> chips = {
        { QStringLiteral("All Products"), QStringLiteral("All") },
        { QStringLiteral("Critical"), QStringLiteral("Critical") },
        { QStringLiteral("Reorder Warning"), QStringLiteral("Reorder") },
        { QStringLiteral("Overstock"), QStringLiteral("Overstock") },
        { QStringLiteral("Safe"), QStringLiteral("Safe") }
    };

    for (const auto& chip : chips) {
        auto* btn = new QPushButton(chip.label, this);
        btn->setObjectName(QStringLiteral("FilterChip"));
        btn->setProperty("value", chip.value);
        if (chip.value == QLatin1String("All")) {
            btn->setProperty("active", true);
        } else {
            btn->setProperty("active", false);
        }
        connect(btn, &QPushButton::clicked, this, &DashboardWidget::onFilterChipClicked);
        m_filterChips.append(btn);
        rowLayout->addWidget(btn);
    }
    rowLayout->addStretch();
}

void DashboardWidget::configureTable() {
    m_model = new ProductModel(this);
    
    auto* filterProxy = new ProductFilterProxyModel(this);
    filterProxy->setSourceModel(m_model);
    filterProxy->setSortRole(Qt::DisplayRole);
    m_proxyModel = filterProxy;
    
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
    m_tableView->setColumnWidth(static_cast<int>(ProductColumn::Name), 220);
    m_tableView->setColumnWidth(static_cast<int>(ProductColumn::DemandLabel), 90);
    m_tableView->setColumnWidth(static_cast<int>(ProductColumn::StockStatus), 110);
    m_tableView->setColumnWidth(static_cast<int>(ProductColumn::Confidence), 90);
    m_tableView->setColumnWidth(static_cast<int>(ProductColumn::Forecast), 100);
    m_tableView->setColumnWidth(static_cast<int>(ProductColumn::Priority), 110);
    m_tableView->setColumnWidth(static_cast<int>(ProductColumn::EOQQty), 85);

    connect(m_tableView, &QTableView::doubleClicked, this, &DashboardWidget::onRowDoubleClicked);
}

void DashboardWidget::updateMetrics() {
    m_cardSKUs->setValue(QString::number(m_controller->products().size()));
    m_cardSKUs->setSubtitle(QStringLiteral("Total active items"));

    m_cardCritical->setValue(QString::number(m_controller->criticalCount()));
    m_cardCritical->setSubtitle(QStringLiteral("Immediate orders needed"));

    m_cardOverstock->setValue(QString::number(m_controller->overstockCount()));
    m_cardOverstock->setSubtitle(QStringLiteral("Excess capital items"));

    m_cardStockout->setValue(QStringLiteral("%1%").arg(m_controller->stockoutRiskPct(), 0, 'f', 1));
    m_cardStockout->setSubtitle(QStringLiteral("Products at stockout risk"));
}

void DashboardWidget::updateActivityFeed() {
    auto* db = m_controller->database();
    if (!db) return;

    m_activityList->clear();
    QVector<StockMovement> movements = db->getRecentMovements(5);

    if (movements.isEmpty()) {
        auto* item = new QListWidgetItem(QStringLiteral("No recent activity recorded."), m_activityList);
        item->setFlags(Qt::NoItemFlags);
        item->setForeground(QColor(Palette::TextSecondary));
        return;
    }

    for (const auto& mv : movements) {
        QString text = QStringLiteral("[%1] %2 %3\nQty: %4 · By: %5")
            .arg(mv.movementType)
            .arg(mv.productName)
            .arg(mv.movementType == QLatin1String("IN") ? QStringLiteral("restocked") : QStringLiteral("removed"))
            .arg(mv.quantity)
            .arg(mv.staffName.isEmpty() ? QStringLiteral("Owner") : mv.staffName);

        auto* item = new QListWidgetItem(text, m_activityList);
        item->setFlags(Qt::ItemIsEnabled);
        
        // Coloring of text item or icon block
        if (mv.movementType == QLatin1String("IN")) {
            item->setForeground(QColor(Palette::Success));
        } else {
            item->setForeground(QColor(Palette::Critical));
        }
    }
}

void DashboardWidget::onProductsChanged() {
    m_model->setProducts(m_controller->products());
    updateMetrics();
    updateActivityFeed();
}

void DashboardWidget::onFilterChipClicked() {
    auto* clicked = qobject_cast<QPushButton*>(sender());
    if (!clicked) return;

    QString val = clicked->property("value").toString();
    
    // Toggle active state in UI properties
    for (auto* btn : m_filterChips) {
        if (btn == clicked) {
            btn->setProperty("active", true);
        } else {
            btn->setProperty("active", false);
        }
        btn->style()->unpolish(btn);
        btn->style()->polish(btn);
        btn->update();
    }

    auto* proxy = static_cast<ProductFilterProxyModel*>(m_proxyModel);
    if (proxy) {
        proxy->filterState = val;
        proxy->invalidate();
    }
}

void DashboardWidget::onRowDoubleClicked(const QModelIndex& index) {
    if (!index.isValid()) return;
    
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
