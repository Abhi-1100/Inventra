#pragma once

#include <QWidget>
#include <QModelIndex>

class QTableView;
class QSortFilterProxyModel;
class QPushButton;
class QLineEdit;
class QLabel;
class QComboBox;

namespace Kirana {

class AppController;
class ProductModel;
class ProductDetailPanel;

// ─────────────────────────────────────────────
// ProductCatalogWidget
//
// Full product catalog page matching 8_Product_Catalog.html:
// - Header: title + "Add Product" CTA button
// - Filter bar: search + category filter + status filter + demand filter
// - Full-width table with: Name/Icon, SKU, Category, Stock, Demand Tier,
//   Status (dot+label), EOQ Rec
// ─────────────────────────────────────────────

class ProductCatalogWidget : public QWidget {
    Q_OBJECT

public:
    explicit ProductCatalogWidget(AppController* controller, QWidget* parent = nullptr);
    ~ProductCatalogWidget() override = default;

    void refresh();

protected:
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onProductsChanged();
    void onRowDoubleClicked(const QModelIndex& index);
    void onSearchTextChanged(const QString& text);
    void onFilterChanged();

private:
    void buildLayout();
    void configureTable();

    AppController*         m_controller = nullptr;
    ProductModel*          m_model      = nullptr;
    QSortFilterProxyModel* m_proxyModel = nullptr;

    QLineEdit*   m_searchEdit  = nullptr;
    QComboBox*   m_catFilter   = nullptr;
    QComboBox*   m_statusFilter= nullptr;
    QComboBox*   m_demandFilter= nullptr;
    QLabel*      m_countLabel  = nullptr;

    QTableView*        m_tableView   = nullptr;
    ProductDetailPanel* m_detailPanel = nullptr;
};

} // namespace Kirana
