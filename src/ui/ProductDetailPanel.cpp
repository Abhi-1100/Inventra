#include "core/ThemeManager.h"
#include "ui/ProductDetailPanel.h"
#include <QPainter>
#include <QPaintEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QPropertyAnimation>
#include <QEvent>
#include <QDateTime>

// QtCharts
#include <QChartView>
#include <QChart>
#include <QLineSeries>
#include <QAreaSeries>
#include <QDateTimeAxis>
#include <QValueAxis>
#include <QGraphicsLayout>

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
QT_CHARTS_USE_NAMESPACE
#endif

namespace Kirana {

ProductDetailPanel::ProductDetailPanel(QWidget* parent)
    : QWidget(parent)
{
    setObjectName("ProductDetailPanel");
    
    // Transparent or dark background styled in constructor and paintEvent
    setAttribute(Qt::WA_NoSystemBackground, true);

    buildLayout();

    // Set up slide-in animation
    m_animation = new QPropertyAnimation(this, "pos", this);
    m_animation->setDuration(300);
    m_animation->setEasingCurve(QEasingCurve::OutCubic);

    // Watch parent events to adjust layout on resize
    if (parent) {
        parent->installEventFilter(this);
    }
}

void ProductDetailPanel::buildLayout() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(16);

    // ── Header Row ────────────────────────────────
    auto* headerLayout = new QHBoxLayout();
    
    auto* infoContainer = new QWidget(this);
    auto* infoLayout = new QVBoxLayout(infoContainer);
    infoLayout->setContentsMargins(0, 0, 0, 0);
    infoLayout->setSpacing(2);

    m_skuLabel = new QLabel(this);
    m_skuLabel->setStyleSheet("font-family: 'Consolas', monospace; font-size: 11px; color: #8b949e;");

    m_nameLabel = new QLabel(this);
    m_nameLabel->setStyleSheet("font-family: 'Segoe UI', sans-serif; font-size: 18px; font-weight: bold; color: #e6edf3;");

    m_categoryLabel = new QLabel(this);
    m_categoryLabel->setStyleSheet("font-family: 'Segoe UI', sans-serif; font-size: 11px; color: #58a6ff; text-transform: uppercase;");

    infoLayout->addWidget(m_skuLabel);
    infoLayout->addWidget(m_nameLabel);
    infoLayout->addWidget(m_categoryLabel);

    m_closeBtn = new QPushButton("×", this);
    m_closeBtn->setFixedSize(30, 30);
    m_closeBtn->setStyleSheet(
        "QPushButton {"
        "  font-size: 20px;"
        "  color: #8b949e;"
        "  background: transparent;"
        "  border: none;"
        "}"
        "QPushButton:hover {"
        "  color: #f85149;"
        "}"
    );
    connect(m_closeBtn, &QPushButton::clicked, this, &ProductDetailPanel::slideOut);

    headerLayout->addWidget(infoContainer, 1);
    headerLayout->addWidget(m_closeBtn);
    mainLayout->addLayout(headerLayout);

    // ── Separator line ────────────────────────────
    auto* line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("background-color: #30363d; max-height: 1px; border: none;");
    mainLayout->addWidget(line);

    // ── Basic Stock Info ──────────────────────────
    m_stockLabel = new QLabel(this);
    m_stockLabel->setStyleSheet("font-family: 'Consolas', monospace; font-size: 13px; color: #e6edf3;");
    mainLayout->addWidget(m_stockLabel);

    // ── Forecast Chart ────────────────────────────
    m_chartView = new QChartView(this);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setFixedHeight(260);
    m_chartView->setStyleSheet("background: transparent; border: 1px solid #30363d; border-radius: 4px;");
    mainLayout->addWidget(m_chartView, 1);

    // ── EOQ Details Box (QFrame) ──────────────────
    auto* eoqFrame = new QFrame(this);
    eoqFrame->setObjectName("EOQCard");
    eoqFrame->setStyleSheet(
        "QFrame#EOQCard {"
        "  background: #161b22;"
        "  border: 1px solid #30363d;"
        "  border-radius: 6px;"
        "  padding: 12px;"
        "}"
    );
    auto* eoqLayout = new QVBoxLayout(eoqFrame);
    eoqLayout->setSpacing(8);

    auto* eoqTitle = new QLabel("ECONOMIC ORDER QUANTITY (EOQ) ASSUMPTIONS", eoqFrame);
    eoqTitle->setStyleSheet("font-size: 10px; font-weight: bold; color: #8b949e; letter-spacing: 0.5px;");
    eoqLayout->addWidget(eoqTitle);

    auto* eoqGrid = new QHBoxLayout();
    
    auto* eoqValContainer = new QVBoxLayout();
    auto* eoqValTitle = new QLabel("RECOMMENDED EOQ", eoqFrame);
    eoqValTitle->setStyleSheet("font-size: 9px; color: #8b949e;");
    m_eoqResultVal = new QLabel("—", eoqFrame);
    m_eoqResultVal->setStyleSheet("font-family: 'Consolas', monospace; font-size: 20px; font-weight: bold; color: #3fb950;");
    eoqValContainer->addWidget(eoqValTitle);
    eoqValContainer->addWidget(m_eoqResultVal);

    auto* costContainer = new QVBoxLayout();
    auto* orderingTitle = new QLabel("ORDERING COST", eoqFrame);
    orderingTitle->setStyleSheet("font-size: 9px; color: #8b949e;");
    m_orderingCostVal = new QLabel("—", eoqFrame);
    m_orderingCostVal->setStyleSheet("font-family: 'Consolas', monospace; font-size: 12px; color: #e6edf3;");
    costContainer->addWidget(orderingTitle);
    costContainer->addWidget(m_orderingCostVal);

    auto* holdingContainer = new QVBoxLayout();
    auto* holdingTitle = new QLabel("HOLDING RATE", eoqFrame);
    holdingTitle->setStyleSheet("font-size: 9px; color: #8b949e;");
    m_holdingCostVal = new QLabel("—", eoqFrame);
    m_holdingCostVal->setStyleSheet("font-family: 'Consolas', monospace; font-size: 12px; color: #e6edf3;");
    holdingContainer->addWidget(holdingTitle);
    holdingContainer->addWidget(m_holdingCostVal);

    eoqGrid->addLayout(eoqValContainer, 2);
    eoqGrid->addLayout(costContainer, 1);
    eoqGrid->addLayout(holdingContainer, 1);
    eoqLayout->addLayout(eoqGrid);

    m_defaultWarning = new QLabel("⚠️ USING DEFAULT COST ASSUMPTIONS", eoqFrame);
    m_defaultWarning->setStyleSheet("font-size: 9px; font-weight: bold; color: #d29922;");
    m_defaultWarning->setVisible(false);
    eoqLayout->addWidget(m_defaultWarning);

    mainLayout->addWidget(eoqFrame);
}

void ProductDetailPanel::setProduct(const Product& product) {
    m_product = product;

    m_skuLabel->setText("SKU: " + m_product.sku);
    m_nameLabel->setText(m_product.name);
    m_categoryLabel->setText(m_product.category);

    m_stockLabel->setText(QString("Current Stock: %1 units   |   Unit Cost: ₹%2")
                              .arg(m_product.currentStock)
                              .arg(m_product.unitCost, 0, 'f', 2));

    updateCharts();
    updateEOQDetails();
}

void ProductDetailPanel::updateCharts() {
    auto* chart = new QChart();
    chart->setBackgroundVisible(false);
    chart->setMargins(QMargins(5, 5, 5, 5));
    chart->layout()->setContentsMargins(0, 0, 0, 0);

    // Dark theme text styling for chart
    QColor textCol = ThemeManager::instance().tokens().TextSecondary;
    QColor gridCol = ThemeManager::instance().tokens().Border;

    // ── Prophet Forecast Line ───────────────────
    auto* forecastSeries = new QLineSeries();
    forecastSeries->setName("Prophet Forecast");
    QPen forecastPen;
    forecastPen.setColor(ThemeManager::instance().tokens().Accent);
    forecastPen.setWidth(2);
    forecastSeries->setPen(forecastPen);

    // Upper and Lower confidence bounds
    auto* upperSeries = new QLineSeries();
    auto* lowerSeries = new QLineSeries();

    QDateTime minDate, maxDate;
    double minY = 999999, maxY = 0;

    for (const auto& pt : m_product.forecast) {
        QDateTime dt = QDateTime(pt.date, QTime(0, 0));
        forecastSeries->append(dt.toMSecsSinceEpoch(), pt.value);
        upperSeries->append(dt.toMSecsSinceEpoch(), pt.upper);
        lowerSeries->append(dt.toMSecsSinceEpoch(), pt.lower);

        if (minDate.isNull() || dt < minDate) minDate = dt;
        if (maxDate.isNull() || dt > maxDate) maxDate = dt;
        if (pt.lower < minY) minY = pt.lower;
        if (pt.upper > maxY) maxY = pt.upper;
    }

    // Area series for confidence band
    auto* areaSeries = new QAreaSeries(upperSeries, lowerSeries);
    areaSeries->setName("Confidence Band (80%)");
    QColor bandColor(ThemeManager::instance().tokens().Accent);
    bandColor.setAlpha(35);
    areaSeries->setBrush(QBrush(bandColor));
    areaSeries->setPen(Qt::NoPen);

    chart->addSeries(areaSeries);
    chart->addSeries(forecastSeries);

    // ── Axis Configuration ─────────────────────
    auto* axisX = new QDateTimeAxis();
    axisX->setFormat("dd MMM");
    axisX->setTitleText("Date");
    axisX->setLabelsColor(textCol);
    axisX->setTitleBrush(QBrush(textCol));
    axisX->setGridLineColor(gridCol);
    axisX->setRange(minDate, maxDate);
    axisX->setTickCount(7);
    chart->addAxis(axisX, Qt::AlignBottom);
    forecastSeries->attachAxis(axisX);
    areaSeries->attachAxis(axisX);

    auto* axisY = new QValueAxis();
    axisY->setTitleText("Demand (Units)");
    axisY->setLabelsColor(textCol);
    axisY->setTitleBrush(QBrush(textCol));
    axisY->setGridLineColor(gridCol);
    axisY->setRange(qMax(0.0, minY * 0.8), maxY * 1.2);
    chart->addAxis(axisY, Qt::AlignLeft);
    forecastSeries->attachAxis(axisY);
    areaSeries->attachAxis(axisY);

    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);
    chart->legend()->setLabelColor(textCol);

    auto* oldChart = m_chartView->chart();
    m_chartView->setChart(chart);
    if (oldChart) delete oldChart;
}

void ProductDetailPanel::updateEOQDetails() {
    m_eoqResultVal->setText(QString("%1 units").arg(m_product.eoqQty));
    m_orderingCostVal->setText(QString("₹%1 /order").arg(m_product.orderingCost, 0, 'f', 1));
    m_holdingCostVal->setText(QString("%1%").arg(m_product.holdingCostRate * 100, 0, 'f', 0));
    m_defaultWarning->setVisible(m_product.usingDefaultEOQ);
}

// ─────────────────────────────────────────────
// Sliding Animation Actions
// ─────────────────────────────────────────────

void ProductDetailPanel::slideIn() {
    m_isSlidOut = false;
    setVisible(true);
    raise();
    adjustPanelPosition();
}

void ProductDetailPanel::slideOut() {
    m_isSlidOut = true;
    
    // Slide right, out of viewport
    auto* parent = parentWidget();
    if (!parent) return;

    m_animation->stop();
    m_animation->setStartValue(pos());
    m_animation->setEndValue(QPoint(parent->width(), 0));
    
    connect(m_animation, &QPropertyAnimation::finished, this, [this]() {
        if (m_isSlidOut) setVisible(false);
    });
    m_animation->start();
}

void ProductDetailPanel::adjustPanelPosition() {
    auto* parent = parentWidget();
    if (!parent || m_isSlidOut) return;

    int panelW = qMin(480, parent->width() - 80);
    setFixedHeight(parent->height());
    setFixedWidth(panelW);

    m_animation->stop();
    m_animation->setStartValue(pos());
    m_animation->setEndValue(QPoint(parent->width() - panelW, 0));
    m_animation->start();
}

// ─────────────────────────────────────────────
// Event Filter and Paint
// ─────────────────────────────────────────────

bool ProductDetailPanel::eventFilter(QObject* watched, QEvent* event) {
    if (watched == parentWidget() && event->type() == QEvent::Resize) {
        adjustPanelPosition();
    }
    return QWidget::eventFilter(watched, event);
}

void ProductDetailPanel::paintEvent(QPaintEvent* /*event*/) {
    QPainter p(this);
    
    // Semi-translucent overlay background to give a "glassmorphic" panel overlay
    p.fillRect(rect(), QColor("#0d1117"));

    // Left border separator
    p.setPen(QColor("#30363d"));
    p.drawLine(0, 0, 0, height());
}

} // namespace Kirana
