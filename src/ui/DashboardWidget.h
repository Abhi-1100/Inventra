#pragma once

#include <QWidget>
#include <QModelIndex>
#include "core/ProductData.h"

class QTableView;
class QSortFilterProxyModel;

namespace Kirana {

class AppController;
class ProductModel;
class MetricCard;
class ProductDetailPanel;

// ─────────────────────────────────────────────
// DashboardWidget
//
// Shows 4 metric cards at the top:
//   - Total SKUs
//   - Critical Reorders
//   - Overstock Items
//   - Stockout Risk %
//
// Below that, a dense QTableView of all products.
// Supports column sorting, row double-click.
// Double-click slides in the ProductDetailPanel.
// ─────────────────────────────────────────────

class DashboardWidget : public QWidget {
    Q_OBJECT

public:
    explicit DashboardWidget(AppController* controller, QWidget* parent = nullptr);
    ~DashboardWidget() override = default;

    void updateMetrics();

protected:
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onRowDoubleClicked(const QModelIndex& index);
    void onProductsChanged();

private:
    void buildLayout();
    void configureTable();

    AppController*        m_controller = nullptr;
    ProductModel*         m_model      = nullptr;
    QSortFilterProxyModel* m_proxyModel = nullptr;

    // Metrics
    MetricCard* m_cardSKUs       = nullptr;
    MetricCard* m_cardCritical   = nullptr;
    MetricCard* m_cardOverstock  = nullptr;
    MetricCard* m_cardStockout   = nullptr;

    QTableView* m_tableView = nullptr;

    // Details panel
    ProductDetailPanel* m_detailPanel = nullptr;
};

} // namespace Kirana
