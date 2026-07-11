#pragma once

#include <QWidget>
#include "core/ProductData.h"

class QChart;
class QChartView;


class QLabel;
class QPushButton;
class QPropertyAnimation;

namespace Kirana {

// ─────────────────────────────────────────────
// ProductDetailPanel
//
// An overlay panel that slides in from the right edge
// of the parent widget to display detailed product statistics,
// including a 7-day Prophet forecast chart and EOQ calculations.
// ─────────────────────────────────────────────

class ProductDetailPanel : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QPoint pos READ pos WRITE move) // Enable QPropertyAnimation on position

public:
    explicit ProductDetailPanel(QWidget* parent = nullptr);
    ~ProductDetailPanel() override = default;

    void setProduct(const Product& product);
    void slideIn();
    void slideOut();

    void adjustPanelPosition();

protected:
    void paintEvent(QPaintEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void buildLayout();
    void updateCharts();
    void updateEOQDetails();

    Product m_product;
    bool m_isSlidOut = true;

    // UI elements
    QLabel* m_skuLabel       = nullptr;
    QLabel* m_nameLabel      = nullptr;
    QLabel* m_categoryLabel  = nullptr;
    QLabel* m_stockLabel     = nullptr;

    // EOQ Card labels
    QLabel* m_eoqResultVal   = nullptr;
    QLabel* m_orderingCostVal= nullptr;
    QLabel* m_holdingCostVal = nullptr;
    QLabel* m_defaultWarning = nullptr;

    // Charts
    QChartView* m_chartView  = nullptr;

    QPushButton* m_closeBtn  = nullptr;
    QPropertyAnimation* m_animation = nullptr;
};

} // namespace Kirana
