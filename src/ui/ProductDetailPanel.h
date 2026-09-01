#pragma once

#include <QWidget>
#include "core/ProductData.h"

class QChart;
class QChartView;
class QLabel;
class QPushButton;
class QPropertyAnimation;
class QScrollArea;

namespace Kirana {

struct AppSettings;

// ─────────────────────────────────────────────
// ProductDetailPanel
//
// An overlay panel that slides in from the right edge
// of the parent widget to display detailed product statistics,
// including all ML prediction outputs (demand label, stock status,
// confidence, forecast, EOQ, priority, recommendation, explanation)
// and a 7-day Prophet forecast chart.
// ─────────────────────────────────────────────

class ProductDetailPanel : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QPoint pos READ pos WRITE move) // Enable QPropertyAnimation on position

public:
    explicit ProductDetailPanel(QWidget* parent = nullptr);
    ~ProductDetailPanel() override = default;

    void setProduct(const Product& product);
    void setSettings(const AppSettings* settings) { m_settings = settings; }
    void slideIn();
    void slideOut();
    void adjustPanelPosition();

signals:
    // Emitted when the "Run Prediction" button is clicked
    void runPredictionRequested(int productId);

protected:
    void paintEvent(QPaintEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    void buildLayout();
    void updateCharts();
    void updateEOQDetails();
    void updateMLDetails();

    Product m_product;
    const AppSettings* m_settings = nullptr;
    bool m_isSlidOut   = true;
    bool m_fetchPending = false;   // true while an API fetch is in-flight

    // ── Header info ──
    QLabel* m_skuLabel       = nullptr;
    QLabel* m_nameLabel      = nullptr;
    QLabel* m_categoryLabel  = nullptr;
    QLabel* m_stockValue     = nullptr;
    QLabel* m_reorderValue   = nullptr;
    QLabel* m_unitCostValue  = nullptr;

    // ── ML Prediction badges ──
    QLabel* m_demandBadge    = nullptr;
    QLabel* m_statusBadge    = nullptr;
    QLabel* m_confidenceVal  = nullptr;
    QLabel* m_forecastVal    = nullptr;
    QLabel* m_chartSummary   = nullptr;
    QLabel* m_priorityBadge  = nullptr;

    // ── EOQ Card labels ──
    QLabel* m_eoqResultVal   = nullptr;
    QLabel* m_orderingCostVal= nullptr;
    QLabel* m_holdingCostVal = nullptr;
    QLabel* m_defaultWarning = nullptr;

    // ── Recommendation & Explanation ──
    QLabel* m_recommendationText = nullptr;
    QLabel* m_explanationText    = nullptr;

    // ── Forecast chart ──
    QChartView* m_chartView  = nullptr;

    // ── Buttons ──
    QPushButton* m_closeBtn    = nullptr;

    QPropertyAnimation* m_animation = nullptr;
};

} // namespace Kirana
