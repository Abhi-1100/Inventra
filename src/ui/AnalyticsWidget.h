#pragma once

#include <QWidget>

class QChartView;
class QLabel;


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
    void showOverviewDetails(const QString& section);

    AppController* m_controller = nullptr;

    QChartView* m_barChartView = nullptr;
    QChartView* m_pieChartView = nullptr;
    QLabel*     m_subtitleLabel = nullptr;
    QLabel*     m_productCountLabel = nullptr;
    QLabel*     m_forecastTotalLabel = nullptr;
    QLabel*     m_reorderCountLabel = nullptr;
};

} // namespace Kirana
