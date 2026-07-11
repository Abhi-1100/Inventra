#pragma once

#include <QWidget>
#include <QModelIndex>
#include "core/ProductData.h"

class QTableView;
class QSortFilterProxyModel;
class QPushButton;
class QListWidget;
class QHBoxLayout;

namespace Kirana {

class AppController;
class ProductModel;
class MetricCard;
class ProductDetailPanel;

// ─────────────────────────────────────────────
// DashboardWidget
//
// Shows 4 metric cards at the top.
// Below cards, a horizontal split:
//   - Left: Dense table of products + filter chip bar
//   - Right: Recent stock movement activity feed
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

    AppController*        m_controller = nullptr;
    ProductModel*         m_model      = nullptr;
    QSortFilterProxyModel* m_proxyModel = nullptr;

    // Metrics
    MetricCard* m_cardSKUs       = nullptr;
    MetricCard* m_cardCritical   = nullptr;
    MetricCard* m_cardOverstock  = nullptr;
    MetricCard* m_cardStockout   = nullptr;

    // Filters
    QVector<QPushButton*> m_filterChips;

    QTableView* m_tableView = nullptr;

    // Activity Feed
    QListWidget* m_activityList = nullptr;

    // Details panel
    ProductDetailPanel* m_detailPanel = nullptr;
};

} // namespace Kirana
