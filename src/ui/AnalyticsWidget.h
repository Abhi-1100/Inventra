#pragma once

#include <QWidget>

#include <QChartView>

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
QT_CHARTS_USE_NAMESPACE
#endif


namespace Kirana {

class AppController;

// ─────────────────────────────────────────────
// AnalyticsWidget
//
// Displays descriptive charts built with QtCharts:
//   - A horizontal or vertical bar chart showing top products by demand
//   - A pie chart showing stock status distribution (Reorder/No Action/Overstock)
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

    AppController* m_controller = nullptr;

    QChartView* m_barChartView = nullptr;
    QChartView* m_pieChartView = nullptr;
};

} // namespace Kirana
