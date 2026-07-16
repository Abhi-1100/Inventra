#pragma once

#include <QWidget>
#include <QChartView>
#include <QTextBrowser>

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
QT_CHARTS_USE_NAMESPACE
#endif

namespace Kirana {

class AppController;
class MetricCard;

// ─────────────────────────────────────────────
// AnalyticsWidget
//
// Displays descriptive charts built with QtCharts:
//   - A horizontal or vertical bar chart showing top products by demand
//   - A pie chart showing stock status distribution (Reorder/No Action/Overstock)
//   - A line chart showing Sales vs Waste Trend (last 7 days)
//   - A donut chart showing Category Breakdown
//   - Visual KPI Cards for Sales, Wastage, Valuation, and Alerts
//   - Dynamic HTML operational summary and restock recommendations
// ─────────────────────────────────────────────

class AnalyticsWidget : public QWidget {
    Q_OBJECT

public:
    explicit AnalyticsWidget(AppController* controller, QWidget* parent = nullptr);
    ~AnalyticsWidget() override = default;

private slots:
    void refreshCharts();

private:
    void buildLayout();
    void renderDemandBarChart();
    void renderStatusPieChart();
    void renderTrendChart();
    void renderCategoryChart();
    void generateReport();

    AppController* m_controller = nullptr;

    // Charts
    QChartView* m_barChartView = nullptr;
    QChartView* m_pieChartView = nullptr;
    QChartView* m_trendChartView = nullptr;
    QChartView* m_categoryChartView = nullptr;

    // KPI Cards
    MetricCard* m_cardSalesVolume = nullptr;
    MetricCard* m_cardWastage      = nullptr;
    MetricCard* m_cardValuation    = nullptr;
    MetricCard* m_cardAlerts       = nullptr;

    // Report
    QTextBrowser* m_reportBrowser  = nullptr;
};

} // namespace Kirana
