#include "ui/AnalyticsWidget.h"
#include "core/AppController.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>

// QtCharts
#include <QChartView>
#include <QChart>
#include <QBarSeries>
#include <QBarSet>
#include <QBarCategoryAxis>
#include <QValueAxis>
#include <QPieSeries>
#include <QPieSlice>

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
QT_CHARTS_USE_NAMESPACE
#endif

namespace Kirana {

AnalyticsWidget::AnalyticsWidget(AppController* controller, QWidget* parent)
    : QWidget(parent)
    , m_controller(controller)
{
    buildLayout();
    
    connect(m_controller, &AppController::productsChanged, this, &AnalyticsWidget::refreshCharts);
    
    refreshCharts(); // Initial rendering
}

void AnalyticsWidget::buildLayout() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);

    // Title Section
    auto* titleLabel = new QLabel("ANALYTICS ENGINE", this);
    titleLabel->setStyleSheet("font-family: 'Segoe UI', sans-serif; font-size: 16px; font-weight: bold; color: #e6edf3; letter-spacing: 0.5px;");
    mainLayout->addWidget(titleLabel);

    // Charts side-by-side or stacked
    auto* chartsLayout = new QHBoxLayout();
    chartsLayout->setSpacing(20);

    m_barChartView = new QChartView(this);
    m_barChartView->setRenderHint(QPainter::Antialiasing);
    m_barChartView->setStyleSheet("background: #161b22; border: 1px solid #30363d; border-radius: 6px;");

    m_pieChartView = new QChartView(this);
    m_pieChartView->setRenderHint(QPainter::Antialiasing);
    m_pieChartView->setStyleSheet("background: #161b22; border: 1px solid #30363d; border-radius: 6px;");

    chartsLayout->addWidget(m_barChartView, 1);
    chartsLayout->addWidget(m_pieChartView, 1);

    mainLayout->addLayout(chartsLayout, 1);
}

void AnalyticsWidget::refreshCharts() {
    renderDemandBarChart();
    renderStatusPieChart();
}

void AnalyticsWidget::renderDemandBarChart() {
    auto* chart = new QChart();
    chart->setTitle("TOP 10 DEMANDED PRODUCTS (7-DAY FORECAST)");
    chart->setTitleBrush(QBrush(QColor("#e6edf3")));
    chart->setBackgroundVisible(false);

    QColor textCol("#8b949e");
    QColor gridCol("#21262d");

    // Fetch and sort top products
    QVector<Product> sortedList = m_controller->products();
    std::sort(sortedList.begin(), sortedList.end(), [](const Product& a, const Product& b) {
        return a.forecastNext7 > b.forecastNext7;
    });

    int limit = qMin(10, static_cast<int>(sortedList.size()));
    
    auto* barSet = new QBarSet("Forecast Demand (Units)");
    barSet->setColor(QColor(Palette::Accent));
    
    QStringList categories;
    for (int i = 0; i < limit; ++i) {
        barSet->append(sortedList[i].forecastNext7);
        // Shorten category string for space
        QString shortName = sortedList[i].name;
        if (shortName.length() > 12) shortName = shortName.left(10) + "..";
        categories << shortName;
    }

    auto* series = new QBarSeries();
    series->append(barSet);
    chart->addSeries(series);

    // X Axis Categories
    auto* axisX = new QBarCategoryAxis();
    axisX->append(categories);
    axisX->setLabelsColor(textCol);
    axisX->setGridLineColor(gridCol);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    // Y Axis Value
    auto* axisY = new QValueAxis();
    axisY->setTitleText("Forecast Units");
    axisY->setLabelsColor(textCol);
    axisY->setTitleBrush(QBrush(textCol));
    axisY->setGridLineColor(gridCol);
    // Find dynamic max
    double maxVal = 0;
    for (int i = 0; i < limit; ++i) maxVal = qMax(maxVal, sortedList[i].forecastNext7);
    axisY->setRange(0, maxVal * 1.15);
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    chart->legend()->setVisible(false);

    auto* oldChart = m_barChartView->chart();
    m_barChartView->setChart(chart);
    if (oldChart) delete oldChart;
}

void AnalyticsWidget::renderStatusPieChart() {
    auto* chart = new QChart();
    chart->setTitle("STOCK STATUS DISTRIBUTION");
    chart->setTitleBrush(QBrush(QColor("#e6edf3")));
    chart->setBackgroundVisible(false);

    QColor textCol("#8b949e");

    int reorderCount = 0;
    int safeCount = 0;
    int overstockCount = 0;

    for (const auto& p : m_controller->products()) {
        if (p.stockStatus == StockStatus::Reorder) reorderCount++;
        else if (p.stockStatus == StockStatus::Overstock) overstockCount++;
        else safeCount++;
    }

    auto* series = new QPieSeries();
    
    auto* sReorder = series->append("Reorder", reorderCount);
    sReorder->setColor(QColor(Palette::Critical));
    sReorder->setLabelColor(textCol);

    auto* sSafe = series->append("No Action", safeCount);
    sSafe->setColor(QColor(Palette::Success));
    sSafe->setLabelColor(textCol);

    auto* sOverstock = series->append("Overstock", overstockCount);
    sOverstock->setColor(QColor(Palette::Warning));
    sOverstock->setLabelColor(textCol);

    chart->addSeries(series);
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);
    chart->legend()->setLabelColor(textCol);

    auto* oldChart = m_pieChartView->chart();
    m_pieChartView->setChart(chart);
    if (oldChart) delete oldChart;
}

} // namespace Kirana
