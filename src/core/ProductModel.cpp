#include "core/ProductModel.h"
#include <algorithm>

namespace Kirana {

// ─────────────────────────────────────────────
// Column headers
// ─────────────────────────────────────────────

static const char* kHeaders[] = {
    "PRODUCT",
    "DEMAND",
    "STATUS",
    "CONFIDENCE",
    "7-DAY FCST",
    "PRIORITY",
    "EOQ QTY"
};

// ─────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────

ProductModel::ProductModel(QObject* parent)
    : QAbstractTableModel(parent)
{}

// ─────────────────────────────────────────────
// Required overrides
// ─────────────────────────────────────────────

int ProductModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return static_cast<int>(m_products.size());
}

int ProductModel::columnCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return static_cast<int>(ProductColumn::_Count);
}

QVariant ProductModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation != Qt::Horizontal) return {};
    if (role == Qt::DisplayRole && section < static_cast<int>(ProductColumn::_Count))
        return QString::fromLatin1(kHeaders[section]);
    return {};
}

// ─────────────────────────────────────────────
// data()
// ─────────────────────────────────────────────

QVariant ProductModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= static_cast<int>(m_products.size()))
        return {};

    const Product&  p   = m_products[index.row()];
    const ProductColumn col = static_cast<ProductColumn>(index.column());

    switch (role) {

    case Qt::DisplayRole:
        return displayData(p, col);

    case Qt::TextAlignmentRole:
        return alignmentData(col);

    case ProductRole::BadgeColor:
        return badgeColorData(p, col);

    case ProductRole::TrendValue:
        if (col == ProductColumn::Forecast)
            return p.forecastTrend;
        return {};

    case ProductRole::ProductIndex:
        return index.row();

    default:
        return {};
    }
}

// ─────────────────────────────────────────────
// displayData — text shown in cells
// ─────────────────────────────────────────────

QVariant ProductModel::displayData(const Product& p, ProductColumn col) const {
    switch (col) {
    case ProductColumn::Name:
        return QString("%1\n%2").arg(p.name, p.sku);
    case ProductColumn::DemandLabel:
        return toString(p.demandLabel);
    case ProductColumn::StockStatus:
        return toString(p.stockStatus);
    case ProductColumn::Confidence:
        return QString("%1%").arg(p.confidence, 0, 'f', 1);
    case ProductColumn::Forecast: {
        // Trend symbol handled separately by delegate; return numeric part
        return QString("%1").arg(p.forecastNext7, 0, 'f', 0);
    }
    case ProductColumn::Priority:
        return toString(p.priority);
    case ProductColumn::EOQQty:
        return QString::number(p.eoqQty);
    default:
        return {};
    }
}

// ─────────────────────────────────────────────
// alignmentData
// ─────────────────────────────────────────────

QVariant ProductModel::alignmentData(ProductColumn col) const {
    switch (col) {
    case ProductColumn::Confidence:
    case ProductColumn::EOQQty:
        return QVariant(Qt::AlignRight | Qt::AlignVCenter);
    case ProductColumn::Forecast:
        return QVariant(Qt::AlignRight | Qt::AlignVCenter);
    default:
        return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
    }
}

// ─────────────────────────────────────────────
// badgeColorData — returns QColor for badge cols
// ─────────────────────────────────────────────

QVariant ProductModel::badgeColorData(const Product& p, ProductColumn col) const {
    switch (col) {
    case ProductColumn::DemandLabel:  return badgeColor(p.demandLabel);
    case ProductColumn::StockStatus:  return badgeColor(p.stockStatus);
    case ProductColumn::Priority:     return badgeColor(p.priority);
    default:
        return {};
    }
}

// ─────────────────────────────────────────────
// Static badge colour helpers
// ─────────────────────────────────────────────

QColor ProductModel::badgeColor(DemandLabel d) {
    switch (d) {
    case DemandLabel::High:   return QColor(Palette::Info);     // blue
    case DemandLabel::Medium: return QColor(Palette::Warning);  // amber
    case DemandLabel::Low:    return QColor(Palette::TextSecondary); // muted grey
    default:                  return QColor(Palette::TextMuted);
    }
}

QColor ProductModel::badgeColor(StockStatus s) {
    switch (s) {
    case StockStatus::Reorder:   return QColor(Palette::Critical);  // red
    case StockStatus::NoAction:  return QColor(Palette::Success);   // green
    case StockStatus::Overstock: return QColor(Palette::Warning);   // amber
    default:                     return QColor(Palette::TextMuted);
    }
}

QColor ProductModel::badgeColor(Priority p) {
    switch (p) {
    case Priority::Critical:    return QColor(Palette::Critical);   // red
    case Priority::ReorderSoon: return QColor(Palette::Warning);    // amber
    case Priority::Safe:        return QColor(Palette::Success);    // green
    default:                    return QColor(Palette::TextMuted);
    }
}

// ─────────────────────────────────────────────
// Data management
// ─────────────────────────────────────────────

void ProductModel::setProducts(QVector<Product> products) {
    beginResetModel();
    m_products = std::move(products);
    endResetModel();
}

void ProductModel::appendProduct(const Product& p) {
    const int row = static_cast<int>(m_products.size());
    beginInsertRows({}, row, row);
    m_products.append(p);
    endInsertRows();
}

void ProductModel::clear() {
    beginResetModel();
    m_products.clear();
    endResetModel();
}

const Product& ProductModel::productAt(int row) const {
    return m_products[row];
}

// ─────────────────────────────────────────────
// sort() — in-place sort with layout change
// ─────────────────────────────────────────────

void ProductModel::sort(int column, Qt::SortOrder order) {
    const ProductColumn col = static_cast<ProductColumn>(column);
    const bool asc = (order == Qt::AscendingOrder);

    emit layoutAboutToBeChanged();

    auto cmp = [&](const Product& a, const Product& b) -> bool {
        switch (col) {
        case ProductColumn::Name:
            return asc ? (a.name < b.name) : (a.name > b.name);
        case ProductColumn::DemandLabel:
            return asc ? (static_cast<int>(a.demandLabel) < static_cast<int>(b.demandLabel))
                       : (static_cast<int>(a.demandLabel) > static_cast<int>(b.demandLabel));
        case ProductColumn::StockStatus:
            return asc ? (static_cast<int>(a.stockStatus) < static_cast<int>(b.stockStatus))
                       : (static_cast<int>(a.stockStatus) > static_cast<int>(b.stockStatus));
        case ProductColumn::Confidence:
            return asc ? (a.confidence < b.confidence) : (a.confidence > b.confidence);
        case ProductColumn::Forecast:
            return asc ? (a.forecastNext7 < b.forecastNext7) : (a.forecastNext7 > b.forecastNext7);
        case ProductColumn::Priority:
            return asc ? (static_cast<int>(a.priority) < static_cast<int>(b.priority))
                       : (static_cast<int>(a.priority) > static_cast<int>(b.priority));
        case ProductColumn::EOQQty:
            return asc ? (a.eoqQty < b.eoqQty) : (a.eoqQty > b.eoqQty);
        default:
            return false;
        }
    };

    std::stable_sort(m_products.begin(), m_products.end(), cmp);
    emit layoutChanged();
}

} // namespace Kirana
