#pragma once

#include <QWidget>
#include <QModelIndex>
#include "core/ProductData.h"

class QTableView;
class QSortFilterProxyModel;
class QPushButton;
class QListWidget;
class QHBoxLayout;
class QLabel;

namespace Kirana {

class AppController;
class ProductModel;
class MetricCard;
class ProductDetailPanel;
class StockDonutWidget;

// ─────────────────────────────────────────────
// DashboardWidget
//
// Layout matching 7_Dashboard.html reference:
//   Top: 4 metric cards in a row
//   Bottom (2/3 + 1/3 split):
//     Left:  Priority Queue table with filter chips
//     Right: Stock Distribution donut + legend
// ─────────────────────────────────────────────

class DashboardWidget : public QWidget {
    Q_OBJECT

public:
    explicit DashboardWidget(AppController* controller, QWidget* parent = nullptr);
    ~DashboardWidget() override = default;

    void updateMetrics();
    void updateActivityFeed();

protected:
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onRowDoubleClicked(const QModelIndex& index);
    void onProductsChanged();
    void onFilterChipClicked();

private:
    void buildLayout();
    void configureTable();
    void setupFilterChips(QHBoxLayout* rowLayout);

    AppController*         m_controller = nullptr;
    ProductModel*          m_model      = nullptr;
    QSortFilterProxyModel* m_proxyModel = nullptr;

    // Metric cards
    MetricCard* m_cardSKUs       = nullptr;
    MetricCard* m_cardCritical   = nullptr;
    MetricCard* m_cardOverstock  = nullptr;
    MetricCard* m_cardStockout   = nullptr;

    // Filter chips
    QVector<QPushButton*> m_filterChips;

    // Priority Queue table
    QTableView* m_tableView = nullptr;

    // Stock Distribution donut
    StockDonutWidget* m_donutWidget      = nullptr;
    QLabel*           m_legendSafe       = nullptr;
    QLabel*           m_legendOverstock  = nullptr;
    QLabel*           m_legendReorder    = nullptr;

    // Product detail side panel
    ProductDetailPanel* m_detailPanel = nullptr;
};

} // namespace Kirana
