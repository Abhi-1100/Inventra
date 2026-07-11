#pragma once

#include <QAbstractTableModel>
#include <QColor>
#include <QFont>
#include "core/ProductData.h"

namespace Kirana {

// ─────────────────────────────────────────────
// Column definitions
// ─────────────────────────────────────────────

enum class ProductColumn : int {
    Name        = 0,   // Product name + SKU
    DemandLabel = 1,   // Badge: High/Medium/Low
    StockStatus = 2,   // Badge: Reorder/No Action/Overstock
    Confidence  = 3,   // "%d.d%"
    Forecast    = 4,   // trend arrow + "N units"
    Priority    = 5,   // Badge: Critical/Reorder Soon/Safe
    EOQQty      = 6,   // integer
    _Count      = 7
};

// ─────────────────────────────────────────────
// Custom model roles
// ─────────────────────────────────────────────

namespace ProductRole {
    static constexpr int BadgeColor   = Qt::UserRole + 1;  // QColor for badge BG
    static constexpr int TrendValue   = Qt::UserRole + 2;  // double (forecastTrend)
    static constexpr int ProductIndex = Qt::UserRole + 3;  // int row in source list
}

// ─────────────────────────────────────────────
// Model
// ─────────────────────────────────────────────

class ProductModel : public QAbstractTableModel {
    Q_OBJECT

public:
    explicit ProductModel(QObject* parent = nullptr);

    // QAbstractTableModel overrides
    int      rowCount   (const QModelIndex& parent = {}) const override;
    int      columnCount(const QModelIndex& parent = {}) const override;
    QVariant data       (const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData (int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    void     sort       (int column, Qt::SortOrder order = Qt::AscendingOrder) override;

    // Data management
    void setProducts(QVector<Product> products);
    void appendProduct(const Product& p);
    void clear();

    const Product& productAt(int row) const;
    int            productCount() const { return static_cast<int>(m_products.size()); }

    // Convenience: badge colour lookup (also used by BadgeDelegate)
    static QColor badgeColor(DemandLabel d);
    static QColor badgeColor(StockStatus s);
    static QColor badgeColor(Priority p);

private:
    QVector<Product> m_products;

    QVariant displayData   (const Product& p, ProductColumn col) const;
    QVariant alignmentData (ProductColumn col) const;
    QVariant badgeColorData(const Product& p, ProductColumn col) const;
};

} // namespace Kirana
