#include "core/ThemeManager.h"
#include "ui/AnalyticsWidget.h"
#include "core/AppController.h"
#include "core/ApiClient.h"          // ---- ADDED: API Integration ----

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QFont>
#include <QPushButton>
#include <QDialog>
#include <QScrollArea>
#include <QMap>
#include <algorithm>
#include <functional>
#include <QMouseEvent>

// QtCharts
#include <QChartView>
#include <QChart>
#include <QBarSeries>
#include <QBarSet>
#include <QBarCategoryAxis>
#include <QValueAxis>
#include <QPieSeries>
#include <QPieSlice>
#include <QJsonObject>               // ---- ADDED: API Integration ----
#include <QJsonArray>                // ---- ADDED: API Integration ----

namespace Kirana {

class ClickableOverviewCard final : public QFrame {
public:
    explicit ClickableOverviewCard(QWidget* parent = nullptr) : QFrame(parent) {
        setCursor(Qt::PointingHandCursor);
    }

    std::function<void()> onActivated;

protected:
    void mouseReleaseEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton && rect().contains(event->pos()) && onActivated)
            onActivated();
        QFrame::mouseReleaseEvent(event);
    }
};

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
    auto* titleLabel = new QLabel(QStringLiteral("Analytics"), this);
    titleLabel->setStyleSheet(QStringLiteral(
        "font-family:'Hanken Grotesk','Segoe UI',sans-serif;"
        "font-size:28px;font-weight:700;color:#e0e2ea;"
        "background:transparent;border:none;"));
    mainLayout->addWidget(titleLabel);

    // Subtitle
    m_subtitleLabel = new QLabel(this);
    m_subtitleLabel->setStyleSheet(QStringLiteral(
        "font-size:14px;color:#8c90a0;font-family:'Hanken Grotesk',sans-serif;"
        "background:transparent;border:none;"));
    mainLayout->addWidget(m_subtitleLabel);

    auto* overviewRow = new QHBoxLayout;
    overviewRow->setSpacing(12);
    auto makeOverviewCard = [this](const QString& title, QLabel*& valueLabel,
                                   const QString& section) {
        auto* card = new ClickableOverviewCard(this);
        card->setToolTip(QStringLiteral("Click to view details"));
        card->setMinimumHeight(84);
        card->setStyleSheet(QStringLiteral(
            "QFrame { background:#161b22; border:1px solid #232a33; border-radius:8px; }"
            "QFrame:hover { background:#1b222c; border-color:#5b82bd; }"));
        auto* layout = new QVBoxLayout(card);
        layout->setContentsMargins(14, 10, 14, 10);
        layout->setSpacing(2);
        auto* heading = new QLabel(title, card);
        heading->setStyleSheet(QStringLiteral(
            "font-size:10px;font-weight:600;letter-spacing:0.8px;color:#8c90a0;"
            "background:transparent;border:none;"));
        heading->setAttribute(Qt::WA_TransparentForMouseEvents);
        valueLabel = new QLabel(card);
        valueLabel->setStyleSheet(QStringLiteral(
            "font-size:20px;font-weight:700;color:#e0e2ea;background:transparent;border:none;"));
        valueLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
        layout->addWidget(heading);
        layout->addWidget(valueLabel);
        card->onActivated = [this, section]() {
            showOverviewDetails(section);
        };
        return card;
    };
    overviewRow->addWidget(makeOverviewCard(QStringLiteral("PRODUCTS ANALYSED"), m_productCountLabel,
                                             QStringLiteral("products")));
    overviewRow->addWidget(makeOverviewCard(QStringLiteral("7-DAY FORECAST"), m_forecastTotalLabel,
                                             QStringLiteral("forecast")));
    overviewRow->addWidget(makeOverviewCard(QStringLiteral("REORDER REQUIRED"), m_reorderCountLabel,
                                             QStringLiteral("reorder")));
    overviewRow->addStretch();
    mainLayout->addLayout(overviewRow);

    // Charts side-by-side or stacked
    auto* chartsLayout = new QHBoxLayout();
    chartsLayout->setSpacing(20);

    m_barChartView = new QChartView(this);
    m_barChartView->setRenderHint(QPainter::Antialiasing);
    m_barChartView->setStyleSheet(
        "background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #161b22,stop:1 #12161c);"
        "border:1px solid #232a33;border-radius:8px;");

    m_pieChartView = new QChartView(this);
    m_pieChartView->setRenderHint(QPainter::Antialiasing);
    m_pieChartView->setStyleSheet(
        "background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #161b22,stop:1 #12161c);"
        "border:1px solid #232a33;border-radius:8px;");

    chartsLayout->addWidget(m_barChartView, 1);
    chartsLayout->addWidget(m_pieChartView, 1);

    mainLayout->addLayout(chartsLayout, 1);
}

void AnalyticsWidget::refreshCharts() {
    const QVector<Product>& products = m_controller->products();
    int reorderCount = 0;
    double forecastTotal = 0.0;
    for (const Product& product : products) {
        forecastTotal += qMax(0.0, product.forecastNext7);
        if (product.stockStatus == StockStatus::Reorder) ++reorderCount;
    }

    m_subtitleLabel->setText(QStringLiteral("Live insights from your current inventory data"));
    m_productCountLabel->setText(QStringLiteral("%1 products").arg(products.size()));
    m_forecastTotalLabel->setText(QStringLiteral("%1 units").arg(qRound(forecastTotal)));
    m_reorderCountLabel->setText(QStringLiteral("%1 products").arg(reorderCount));
    renderDemandBarChart();
    renderStatusPieChart();
}

void AnalyticsWidget::renderDemandBarChart() {
    auto* chart = new QChart();
    chart->setTitle(QStringLiteral("Top Products by 7-Day Forecast"));
    chart->setTitleBrush(QBrush(QColor("#e0e2ea")));
    chart->setTitleFont(QFont(QStringLiteral("Hanken Grotesk"), 14, QFont::DemiBold));
    chart->setBackgroundVisible(false);
    chart->setMargins(QMargins(8, 8, 8, 8));
    chart->setAnimationOptions(QChart::SeriesAnimations);

    QColor textCol("#8c90a0");
    QColor gridCol("#232a33");

    // Fetch and sort top products
    QVector<Product> sortedList = m_controller->products();
    std::sort(sortedList.begin(), sortedList.end(), [](const Product& a, const Product& b) {
        return a.forecastNext7 > b.forecastNext7;
    });

    int limit = qMin(10, static_cast<int>(sortedList.size()));
    
    auto* barSet = new QBarSet("7-Day Forecast");
    barSet->setColor(ThemeManager::instance().tokens().Accent);
    
    QStringList categories;
    for (int i = 0; i < limit; ++i) {
        barSet->append(sortedList[i].forecastNext7);
        // Shorten category string for space
        QString shortName = sortedList[i].name;
        if (shortName.length() > 14) shortName = shortName.left(13) + QStringLiteral("…");
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
    axisY->setLabelFormat(QStringLiteral("%.0f"));
    axisY->setRange(0, qMax(1.0, maxVal * 1.15));
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    chart->legend()->setVisible(false);

    auto* oldChart = m_barChartView->chart();
    m_barChartView->setChart(chart);
    if (oldChart) delete oldChart;
}

void AnalyticsWidget::renderStatusPieChart() {
    auto* chart = new QChart();
    chart->setTitle("Inventory Status Distribution");
    chart->setTitleBrush(QBrush(QColor("#e6edf3")));
    chart->setTitleFont(QFont(QStringLiteral("Hanken Grotesk"), 14, QFont::DemiBold));
    chart->setBackgroundVisible(false);
    chart->setMargins(QMargins(8, 8, 8, 8));
    chart->setAnimationOptions(QChart::SeriesAnimations);

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
    
    auto* sReorder = series->append(QStringLiteral("Reorder (%1)").arg(reorderCount), reorderCount);
    sReorder->setColor(QColor("#ff7b72"));
    sReorder->setLabelColor(textCol);

    auto* sSafe = series->append(QStringLiteral("In Stock (%1)").arg(safeCount), safeCount);
    sSafe->setColor(QColor("#8fb8ff"));
    sSafe->setLabelColor(textCol);

    auto* sOverstock = series->append(QStringLiteral("Overstock (%1)").arg(overstockCount), overstockCount);
    sOverstock->setColor(QColor("#f0b45d"));
    sOverstock->setLabelColor(textCol);

    chart->addSeries(series);
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);
    chart->legend()->setLabelColor(textCol);

    auto* oldChart = m_pieChartView->chart();
    m_pieChartView->setChart(chart);
    if (oldChart) delete oldChart;
}

void AnalyticsWidget::showOverviewDetails(const QString& section) {
    const QVector<Product>& products = m_controller->products();
    QString title;
    QString description;
    QStringList details;

    if (section == QLatin1String("products")) {
        title = QStringLiteral("Products Analysed");
        description = QStringLiteral("All %1 products currently loaded in your inventory, grouped by category.")
                          .arg(products.size());
        QMap<QString, int> categories;
        for (const Product& product : products)
            categories[product.category.isEmpty() ? QStringLiteral("Uncategorised") : product.category]++;
        for (auto it = categories.cbegin(); it != categories.cend(); ++it)
            details.append(QStringLiteral("%1  ·  %2 products").arg(it.key()).arg(it.value()));
    } else if (section == QLatin1String("forecast")) {
        title = QStringLiteral("7-Day Forecast Details");
        QVector<Product> ranked = products;
        std::sort(ranked.begin(), ranked.end(), [](const Product& left, const Product& right) {
            return left.forecastNext7 > right.forecastNext7;
        });
        double total = 0.0;
        for (const Product& product : products) total += qMax(0.0, product.forecastNext7);
        description = QStringLiteral("Forecast demand totals %1 units across all loaded products. Top forecasted products:")
                          .arg(qRound(total));
        const int limit = qMin(10, static_cast<int>(ranked.size()));
        for (int i = 0; i < limit; ++i)
            details.append(QStringLiteral("%1.  %2  ·  %3 units")
                           .arg(i + 1).arg(ranked[i].name).arg(qRound(ranked[i].forecastNext7)));
    } else {
        title = QStringLiteral("Products Requiring Reorder");
        for (const Product& product : products) {
            if (product.stockStatus == StockStatus::Reorder) {
                details.append(QStringLiteral("%1  ·  %2 in stock  ·  reorder point %3")
                               .arg(product.name).arg(product.currentStock).arg(product.reorderPoint));
            }
        }
        description = details.isEmpty()
            ? QStringLiteral("No products currently require a reorder.")
            : QStringLiteral("%1 products are below their reorder threshold.").arg(details.size());
    }

    auto* dialog = new QDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle(title);
    dialog->setMinimumSize(560, 400);
    dialog->resize(620, 500);
    dialog->setStyleSheet(QStringLiteral(
        "QDialog { background:#101419; }"
        "QScrollArea { border:1px solid #232a33; border-radius:8px; background:#161b22; }"));

    auto* layout = new QVBoxLayout(dialog);
    layout->setContentsMargins(24, 22, 24, 20);
    layout->setSpacing(14);

    auto* titleLabel = new QLabel(title, dialog);
    titleLabel->setStyleSheet(QStringLiteral("font-size:22px;font-weight:700;color:#e0e2ea;background:transparent;"));
    auto* descriptionLabel = new QLabel(description, dialog);
    descriptionLabel->setWordWrap(true);
    descriptionLabel->setStyleSheet(QStringLiteral("font-size:13px;color:#aab2c2;background:transparent;"));
    layout->addWidget(titleLabel);
    layout->addWidget(descriptionLabel);

    auto* scrollArea = new QScrollArea(dialog);
    scrollArea->setWidgetResizable(true);
    auto* detailContent = new QWidget(scrollArea);
    auto* detailsLayout = new QVBoxLayout(detailContent);
    detailsLayout->setContentsMargins(12, 12, 12, 12);
    detailsLayout->setSpacing(7);
    for (const QString& detail : details) {
        auto* row = new QLabel(detail, detailContent);
        row->setWordWrap(true);
        row->setStyleSheet(QStringLiteral(
            "background:#1b222c;border:1px solid #2b3542;border-radius:6px;"
            "padding:10px 12px;color:#e0e2ea;font-size:13px;"));
        detailsLayout->addWidget(row);
    }
    if (details.isEmpty()) detailsLayout->addStretch();
    scrollArea->setWidget(detailContent);
    layout->addWidget(scrollArea, 1);

    auto* closeButton = new QPushButton(QStringLiteral("Close"), dialog);
    closeButton->setCursor(Qt::PointingHandCursor);
    closeButton->setFixedHeight(34);
    closeButton->setStyleSheet(QStringLiteral(
        "QPushButton { background:#2b6cbf;border:none;border-radius:6px;color:white;font-weight:600;padding:0 18px; }"
        "QPushButton:hover { background:#3b82d0; }"));
    connect(closeButton, &QPushButton::clicked, dialog, &QDialog::accept);
    layout->addWidget(closeButton, 0, Qt::AlignRight);

    dialog->exec();
}

} // namespace Kirana
