#include "ui/ProductDetailPanel.h"
#include "core/ApiClient.h" // ---- ADDED: API Integration ----
#include "core/AppController.h"
#include "core/ThemeManager.h"
#include <QDateTime>
#include <QEvent>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QJsonArray>  // ---- ADDED: API Integration ----
#include <QJsonObject> // ---- ADDED: API Integration ----
#include <QLabel>
#include <QPaintEvent>
#include <QPainter>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QScrollArea>
#include <QTimer>
#include <QVBoxLayout>

// QtCharts
#include <QAreaSeries>
#include <QChart>
#include <QChartView>
#include <QDateTimeAxis>
#include <QGraphicsLayout>
#include <QLineSeries>
#include <QValueAxis>

namespace Kirana {

// ─────────────────────────────────────────────
// Helper: make a color badge label
// ─────────────────────────────────────────────

static QLabel *makeBadge(const QString &text, const QString &bg,
                         const QString &fg, QWidget *parent) {
  auto *lbl = new QLabel(text, parent);
  lbl->setAlignment(Qt::AlignCenter);
  lbl->setContentsMargins(8, 3, 8, 3);
  lbl->setStyleSheet(QString("QLabel {"
                             "  font-size: 11px; font-weight: bold;"
                             "  background: %1; color: %2;"
                             "  border-radius: 4px;"
                             "}")
                         .arg(bg, fg));
  return lbl;
}

static QLabel *makeStatRow(const QString &label, const QString &value,
                           QWidget *parent) {
  auto *container = new QLabel(parent);
  container->setStyleSheet("QLabel { color: #e6edf3; font-size: 12px; }");
  container->setText(
      QString("<span style='color:#8b949e;'>%1</span>  %2").arg(label, value));
  return container;
}

// ─────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────

ProductDetailPanel::ProductDetailPanel(QWidget *parent) : QWidget(parent) {
  setObjectName("ProductDetailPanel");
  setAttribute(Qt::WA_NoSystemBackground, true);

  buildLayout();

  m_animation = new QPropertyAnimation(this, "pos", this);
  m_animation->setDuration(300);
  m_animation->setEasingCurve(QEasingCurve::OutCubic);

  if (parent) {
    parent->installEventFilter(this);
  }

  setFocusPolicy(Qt::StrongFocus);

  // ── productResultReady: API returned data for a single product ──────────
  connect(
      &ApiClient::instance(), &ApiClient::productResultReady, this,
      [this](const QJsonObject &obj) {
        QString sku = obj.value(QStringLiteral("sku")).toString();
        if (sku.isEmpty())
          sku = obj.value(QStringLiteral("product_id")).toString();

        // Only apply if this result is for the currently displayed product
        if (sku != m_product.sku && QString::number(m_product.id) != sku)
          return;

        m_fetchPending = false; // fetch is done — clear loading gate

        // Check the server's 'not_found' sentinel (graceful 200 with no data)
        bool notFound = obj.value(QStringLiteral("not_found")).toBool(false);
        if (notFound) {
          // Server confirmed: no pipeline data for this SKU
          if (m_product.recommendation.isEmpty())
            m_product.recommendation =
                QStringLiteral("No recommendation available.");
          if (m_product.explanation.isEmpty())
            m_product.explanation =
                QStringLiteral("No detailed explanation available.");
          updateMLDetails();
          return;
        }

        // Keep the displayed inventory facts aligned with the same dataset row
        // that produced this prediction, rather than showing stale local
        // values.
        if (obj.contains(QStringLiteral("current_stock")))
          m_product.currentStock =
              qRound(obj.value(QStringLiteral("current_stock")).toDouble());
        if (obj.contains(QStringLiteral("reorder_point")))
          m_product.reorderPoint =
              qRound(obj.value(QStringLiteral("reorder_point")).toDouble());
        if (obj.contains(QStringLiteral("unit_cost")))
          m_product.unitCost =
              obj.value(QStringLiteral("unit_cost")).toDouble();

        m_stockValue->setText(
            QStringLiteral("%1 units").arg(m_product.currentStock));
        m_reorderValue->setText(
            QStringLiteral("%1 units").arg(m_product.reorderPoint));
        m_unitCostValue->setText(
            QStringLiteral("₹%1").arg(m_product.unitCost, 0, 'f', 2));

        // Apply enriched ML fields (only if the new value is non-empty /
        // non-zero)
        double conf = obj.value(QStringLiteral("confidence")).toDouble();
        if (conf > 0.0)
          m_product.confidence = (conf <= 1.0) ? conf * 100.0 : conf;

        double fc = obj.value(QStringLiteral("forecast_7d")).toDouble();
        if (fc == 0.0)
          fc = obj.value(QStringLiteral("weekly_forecast")).toDouble();
        if (fc > 0.0)
          m_product.forecastNext7 = fc;

        int eoq = obj.value(QStringLiteral("eoq")).toInt();
        if (eoq == 0)
          eoq = qRound(obj.value(QStringLiteral("eoq_quantity")).toDouble());
        if (eoq > 0) {
          m_product.eoqQty = eoq;
          m_product.usingDefaultEOQ = false;
        }

        const QJsonArray points =
            obj.value(QStringLiteral("forecast_points")).toArray();
        if (!points.isEmpty()) {
          m_product.forecast.clear();
          for (const QJsonValue &pointValue : points) {
            const QJsonObject point = pointValue.toObject();
            ForecastPoint forecastPoint;
            forecastPoint.date =
                QDate::fromString(point.value(QStringLiteral("ds")).toString(),
                                  QStringLiteral("yyyy-MM-dd"));
            forecastPoint.value =
                point.value(QStringLiteral("yhat")).toDouble();
            forecastPoint.lower =
                qMax(0.0, point.value(QStringLiteral("yhat_lower")).toDouble());
            forecastPoint.upper =
                qMax(forecastPoint.value,
                     point.value(QStringLiteral("yhat_upper")).toDouble());
            if (forecastPoint.date.isValid())
              m_product.forecast.append(forecastPoint);
          }
        }

        QString dl = obj.value(QStringLiteral("demand_label")).toString();
        if (dl.contains(QStringLiteral("High"), Qt::CaseInsensitive))
          m_product.demandLabel = DemandLabel::High;
        else if (dl.contains(QStringLiteral("Low"), Qt::CaseInsensitive))
          m_product.demandLabel = DemandLabel::Low;
        else if (dl.contains(QStringLiteral("Medium"), Qt::CaseInsensitive))
          m_product.demandLabel = DemandLabel::Medium;

        QString ss = obj.value(QStringLiteral("stock_status")).toString();
        if (ss.contains(QStringLiteral("Critical"), Qt::CaseInsensitive) ||
            ss.contains(QStringLiteral("Reorder"), Qt::CaseInsensitive) ||
            ss.contains(QStringLiteral("Low"), Qt::CaseInsensitive))
          m_product.stockStatus = StockStatus::Reorder;
        else if (ss.contains(QStringLiteral("Overstock"), Qt::CaseInsensitive))
          m_product.stockStatus = StockStatus::Overstock;
        else if (!ss.isEmpty())
          m_product.stockStatus = StockStatus::NoAction;

        QString pr = obj.value(QStringLiteral("priority")).toString();
        if (pr.isEmpty())
          pr = obj.value(QStringLiteral("urgency")).toString();
        if (pr.contains(QStringLiteral("Critical"), Qt::CaseInsensitive))
          m_product.priority = Priority::Critical;
        else if (pr.contains(QStringLiteral("Reorder"), Qt::CaseInsensitive))
          m_product.priority = Priority::ReorderSoon;
        else if (!pr.isEmpty())
          m_product.priority = Priority::Safe;

        // Text fields — prefer non-empty incoming value
        QString rec = obj.value(QStringLiteral("recommendation")).toString();
        if (!rec.isEmpty())
          m_product.recommendation = rec;
        if (m_product.recommendation.isEmpty())
          m_product.recommendation =
              QStringLiteral("No recommendation available.");

        QString exp = obj.value(QStringLiteral("explanation")).toString();
        if (exp.isEmpty())
          exp = obj.value(QStringLiteral("explanations")).toString();
        if (!exp.isEmpty())
          m_product.explanation = exp;
        if (m_product.explanation.isEmpty())
          m_product.explanation =
              QStringLiteral("No detailed explanation available.");

        updateMLDetails();
        updateCharts();
        updateEOQDetails();
      });

  // ── apiError: network/server failure → clear loading state ───────────────
  connect(&ApiClient::instance(), &ApiClient::apiError, this,
          [this](const QString & /*errorMessage*/) {
            if (!m_fetchPending)
              return; // wasn't our fetch
            m_fetchPending = false;
            if (m_product.recommendation.isEmpty())
              m_product.recommendation =
                  QStringLiteral("No recommendation available.");
            if (m_product.explanation.isEmpty())
              m_product.explanation =
                  QStringLiteral("No detailed explanation available.");
            updateMLDetails();
          });
}

// ─────────────────────────────────────────────
// buildLayout
// ─────────────────────────────────────────────

void ProductDetailPanel::buildLayout() {
  // Outer scroll area so the panel works at any screen height
  auto *scrollArea = new QScrollArea(this);
  scrollArea->setWidgetResizable(true);
  scrollArea->setFrameShape(QFrame::NoFrame);
  scrollArea->setStyleSheet(
      "QScrollArea { background: transparent; border: none; }"
      "QScrollBar:vertical { width: 6px; background: #0d1117; }"
      "QScrollBar::handle:vertical { background: #30363d; border-radius: 3px; "
      "}");

  auto *outer = new QVBoxLayout(this);
  outer->setContentsMargins(0, 0, 0, 0);
  outer->addWidget(scrollArea);

  auto *content = new QWidget();
  content->setStyleSheet("background: transparent;");
  scrollArea->setWidget(content);

  auto *mainLayout = new QVBoxLayout(content);
  mainLayout->setContentsMargins(20, 20, 20, 20);
  mainLayout->setSpacing(14);

  // ── Header Row ────────────────────────────────
  auto *headerLayout = new QHBoxLayout();

  auto *infoContainer = new QWidget(content);
  auto *infoLayout = new QVBoxLayout(infoContainer);
  infoLayout->setContentsMargins(0, 0, 0, 0);
  infoLayout->setSpacing(2);

  m_skuLabel = new QLabel(content);
  m_skuLabel->setStyleSheet(
      "font-family: 'Consolas', monospace; font-size: 11px; color: #8b949e;");

  m_nameLabel = new QLabel(content);
  m_nameLabel->setStyleSheet(
      "font-size: 17px; font-weight: bold; color: #e6edf3;");
  m_nameLabel->setWordWrap(true);

  m_categoryLabel = new QLabel(content);
  m_categoryLabel->setStyleSheet("font-size: 11px; color: #58a6ff;");

  infoLayout->addWidget(m_skuLabel);
  infoLayout->addWidget(m_nameLabel);
  infoLayout->addWidget(m_categoryLabel);

  m_closeBtn = new QPushButton("×", content);
  m_closeBtn->setFixedSize(26, 26);
  m_closeBtn->setCursor(Qt::PointingHandCursor);
  m_closeBtn->setStyleSheet(
      "QPushButton { font-size:18px; font-weight:600; color:#ffffff; "
      "background:#da3633;"
      "border:1px solid #f85149; border-radius:13px; padding:0 0 1px 0; }"
      "QPushButton:hover { background:#f85149; border-color:#ff7b72; }");
  connect(m_closeBtn, &QPushButton::clicked, this,
          &ProductDetailPanel::slideOut);

  headerLayout->addWidget(infoContainer, 1);
  headerLayout->addWidget(m_closeBtn, 0, Qt::AlignTop);
  mainLayout->addLayout(headerLayout);

  // ── Inventory snapshot ─────────────────────────
  auto *inventoryCard = new QFrame(content);
  inventoryCard->setObjectName("InventorySnapshot");
  inventoryCard->setStyleSheet(
      "QFrame#InventorySnapshot { background:#111923; border:1px solid "
      "#293746; border-radius:10px; padding:8px; }");
  auto *inventoryLayout = new QHBoxLayout(inventoryCard);
  inventoryLayout->setContentsMargins(10, 8, 10, 8);
  inventoryLayout->setSpacing(8);
  auto makeInventoryMetric = [&](const QString &title, QLabel *&value) {
    auto *metric = new QFrame(inventoryCard);
    metric->setStyleSheet("background:#192431;border:none;border-radius:7px;");
    auto *metricLayout = new QVBoxLayout(metric);
    metricLayout->setContentsMargins(10, 7, 10, 7);
    metricLayout->setSpacing(1);
    auto *label = new QLabel(title, metric);
    label->setStyleSheet("font-size:9px;font-weight:600;letter-spacing:0.6px;"
                         "color:#8f9caf;background:transparent;");
    value = new QLabel(metric);
    value->setStyleSheet(
        "font-size:16px;font-weight:700;color:#edf3fb;background:transparent;");
    metricLayout->addWidget(label);
    metricLayout->addWidget(value);
    inventoryLayout->addWidget(metric, 1);
  };
  makeInventoryMetric("CURRENT STOCK", m_stockValue);
  makeInventoryMetric("REORDER AT", m_reorderValue);
  makeInventoryMetric("UNIT COST", m_unitCostValue);
  mainLayout->addWidget(inventoryCard);

  // ── ML Prediction Results Card ────────────────
  auto *mlCard = new QFrame(content);
  mlCard->setObjectName("MLCard");
  mlCard->setStyleSheet("QFrame#MLCard {"
                        "  background: #121b26;"
                        "  border: 1px solid #314154;"
                        "  border-radius: 10px;"
                        "  padding: 12px;"
                        "}");
  auto *mlLayout = new QVBoxLayout(mlCard);
  mlLayout->setSpacing(10);

  auto *mlTitle = new QLabel("PREDICTION SNAPSHOT", mlCard);
  mlTitle->setStyleSheet("font-size:10px;font-weight:bold;color:#aab9cc;letter-"
                         "spacing:0.8px;background:transparent;");
  mlLayout->addWidget(mlTitle);

  // Row 1: Demand + Status badges
  auto *badgeRow = new QHBoxLayout();
  auto *demandContainer = new QVBoxLayout();
  auto *demandTitle = new QLabel("DEMAND", mlCard);
  demandTitle->setStyleSheet("font-size: 9px; color: #8b949e;");
  m_demandBadge = makeBadge("—", "#21262d", "#8b949e", mlCard);
  demandContainer->addWidget(demandTitle);
  demandContainer->addWidget(m_demandBadge);

  auto *statusContainer = new QVBoxLayout();
  auto *statusTitle = new QLabel("STOCK STATUS", mlCard);
  statusTitle->setStyleSheet("font-size: 9px; color: #8b949e;");
  m_statusBadge = makeBadge("—", "#21262d", "#8b949e", mlCard);
  statusContainer->addWidget(statusTitle);
  statusContainer->addWidget(m_statusBadge);

  auto *priorityContainer = new QVBoxLayout();
  auto *priorityTitle = new QLabel("PRIORITY", mlCard);
  priorityTitle->setStyleSheet("font-size: 9px; color: #8b949e;");
  m_priorityBadge = makeBadge("—", "#21262d", "#8b949e", mlCard);
  priorityContainer->addWidget(priorityTitle);
  priorityContainer->addWidget(m_priorityBadge);

  badgeRow->addLayout(demandContainer);
  badgeRow->addLayout(statusContainer);
  badgeRow->addLayout(priorityContainer);
  mlLayout->addLayout(badgeRow);

  // Row 2: Confidence + Forecast
  auto *statsRow = new QHBoxLayout();

  auto *confContainer = new QVBoxLayout();
  auto *confTitle = new QLabel("CONFIDENCE", mlCard);
  confTitle->setStyleSheet("font-size: 9px; color: #8b949e;");
  m_confidenceVal = new QLabel("—", mlCard);
  m_confidenceVal->setStyleSheet(
      "font-family: 'Consolas', monospace; font-size: 18px; font-weight: bold; "
      "color: #3fb950;");
  confContainer->addWidget(confTitle);
  confContainer->addWidget(m_confidenceVal);

  auto *fcastContainer = new QVBoxLayout();
  auto *fcastTitle = new QLabel("7-DAY FORECAST", mlCard);
  fcastTitle->setStyleSheet("font-size: 9px; color: #8b949e;");
  m_forecastVal = new QLabel("—", mlCard);
  m_forecastVal->setStyleSheet("font-family: 'Consolas', monospace; font-size: "
                               "18px; font-weight: bold; color: #58a6ff;");
  fcastContainer->addWidget(fcastTitle);
  fcastContainer->addWidget(m_forecastVal);

  statsRow->addLayout(confContainer);
  statsRow->addLayout(fcastContainer);
  mlLayout->addLayout(statsRow);
  mainLayout->addWidget(mlCard);

  // ── Recommendation Box ────────────────────────
  auto *recCard = new QFrame(content);
  recCard->setObjectName("RecCard");
  recCard->setStyleSheet("QFrame#RecCard {"
                         "  background: #10263b;"
                         "  border: 1px solid #2366a8;"
                         "  border-radius: 10px;"
                         "  padding: 12px;"
                         "}");
  auto *recLayout = new QVBoxLayout(recCard);
  recLayout->setSpacing(6);

  auto *recTitle = new QLabel("RECOMMENDED ACTION", recCard);
  recTitle->setStyleSheet("font-size:10px;font-weight:bold;color:#65b5ff;"
                          "letter-spacing:0.8px;background:transparent;");
  recLayout->addWidget(recTitle);

  m_recommendationText =
      new QLabel("Run prediction to see recommendation.", recCard);
  m_recommendationText->setWordWrap(true);
  m_recommendationText->setStyleSheet(
      "font-size: 12px; color: #cdd9e5; line-height: 1.5;");
  recLayout->addWidget(m_recommendationText);
  mainLayout->addWidget(recCard);

  // ── Forecast Chart ────────────────────────────
  auto *forecastCard = new QFrame(content);
  forecastCard->setObjectName("ForecastCard");
  forecastCard->setStyleSheet(
      "QFrame#ForecastCard { background:#161b22; border:1px solid #30363d; "
      "border-radius:8px; padding:10px; }");
  auto *forecastLayout = new QVBoxLayout(forecastCard);
  forecastLayout->setContentsMargins(12, 10, 12, 10);
  forecastLayout->setSpacing(4);
  auto *forecastHeader = new QHBoxLayout;
  auto *forecastTitle = new QLabel("7-DAY MODEL FORECAST", forecastCard);
  forecastTitle->setStyleSheet("font-size:10px;font-weight:bold;color:#aab4c3;"
                               "letter-spacing:0.7px;background:transparent;");
  m_chartSummary = new QLabel(forecastCard);
  m_chartSummary->setStyleSheet(
      "font-size:10px;color:#58a6ff;background:transparent;");
  forecastHeader->addWidget(forecastTitle);
  forecastHeader->addStretch();
  forecastHeader->addWidget(m_chartSummary);
  forecastLayout->addLayout(forecastHeader);

  m_chartView = new QChartView(forecastCard);
  m_chartView->setRenderHint(QPainter::Antialiasing);
  m_chartView->setFixedHeight(210);
  m_chartView->setStyleSheet("background: transparent; border: none;");
  forecastLayout->addWidget(m_chartView);
  mainLayout->addWidget(forecastCard);

  // ── EOQ Details ───────────────────────────────
  auto *eoqFrame = new QFrame(content);
  eoqFrame->setObjectName("EOQCard");
  eoqFrame->setStyleSheet("QFrame#EOQCard {"
                          "  background: #121b26;"
                          "  border: 1px solid #314154;"
                          "  border-radius: 10px;"
                          "  padding: 12px;"
                          "}");
  auto *eoqLayout = new QVBoxLayout(eoqFrame);
  eoqLayout->setSpacing(8);

  auto *eoqTitle = new QLabel("REPLENISHMENT PLAN · EOQ", eoqFrame);
  eoqTitle->setStyleSheet("font-size:10px;font-weight:bold;color:#aab9cc;"
                          "letter-spacing:0.8px;background:transparent;");
  eoqLayout->addWidget(eoqTitle);

  auto *eoqGrid = new QHBoxLayout();

  auto *eoqValContainer = new QVBoxLayout();
  auto *eoqValTitle = new QLabel("RECOMMENDED EOQ", eoqFrame);
  eoqValTitle->setStyleSheet("font-size: 9px; color: #8b949e;");
  m_eoqResultVal = new QLabel("—", eoqFrame);
  m_eoqResultVal->setStyleSheet(
      "font-family: 'Consolas', monospace; font-size: 20px; font-weight: bold; "
      "color: #3fb950;");
  eoqValContainer->addWidget(eoqValTitle);
  eoqValContainer->addWidget(m_eoqResultVal);

  auto *costContainer = new QVBoxLayout();
  auto *orderingTitle = new QLabel("ORDERING COST", eoqFrame);
  orderingTitle->setStyleSheet("font-size: 9px; color: #8b949e;");
  m_orderingCostVal = new QLabel("—", eoqFrame);
  m_orderingCostVal->setStyleSheet(
      "font-family: 'Consolas', monospace; font-size: 12px; color: #e6edf3;");
  costContainer->addWidget(orderingTitle);
  costContainer->addWidget(m_orderingCostVal);

  auto *holdingContainer = new QVBoxLayout();
  auto *holdingTitle = new QLabel("HOLDING RATE", eoqFrame);
  holdingTitle->setStyleSheet("font-size: 9px; color: #8b949e;");
  m_holdingCostVal = new QLabel("—", eoqFrame);
  m_holdingCostVal->setStyleSheet(
      "font-family: 'Consolas', monospace; font-size: 12px; color: #e6edf3;");
  holdingContainer->addWidget(holdingTitle);
  holdingContainer->addWidget(m_holdingCostVal);

  eoqGrid->addLayout(eoqValContainer, 2);
  eoqGrid->addLayout(costContainer, 1);
  eoqGrid->addLayout(holdingContainer, 1);
  eoqLayout->addLayout(eoqGrid);

  m_defaultWarning = new QLabel("⚠️ USING DEFAULT COST ASSUMPTIONS", eoqFrame);
  m_defaultWarning->setStyleSheet(
      "font-size: 9px; font-weight: bold; color: #d29922;");
  m_defaultWarning->setVisible(false);
  eoqLayout->addWidget(m_defaultWarning);
  mainLayout->addWidget(eoqFrame);

  // ── Explanation Box ───────────────────────────
  auto *expCard = new QFrame(content);
  expCard->setObjectName("ExpCard");
  expCard->setStyleSheet("QFrame#ExpCard {"
                         "  background: #121b26;"
                         "  border: 1px solid #314154;"
                         "  border-radius: 10px;"
                         "  padding: 12px;"
                         "}");
  auto *expLayout = new QVBoxLayout(expCard);
  expLayout->setSpacing(6);

  auto *expTitle = new QLabel("MODEL EXPLANATION", expCard);
  expTitle->setStyleSheet("font-size:10px;font-weight:bold;color:#aab9cc;"
                          "letter-spacing:0.8px;background:transparent;");
  expLayout->addWidget(expTitle);

  m_explanationText = new QLabel("Run prediction to see explanation.", expCard);
  m_explanationText->setWordWrap(true);
  m_explanationText->setStyleSheet(
      "font-size: 11px; color: #8b949e; line-height: 1.6;");
  expLayout->addWidget(m_explanationText);
  mainLayout->addWidget(expCard);

  // m_runPredBtn removed.
  mainLayout->addStretch();
}

// ─────────────────────────────────────────────
// setProduct
// ─────────────────────────────────────────────

void ProductDetailPanel::setProduct(const Product &product) {
  m_product = product;
  m_fetchPending = false; // reset any previous pending state

  m_skuLabel->setText(QStringLiteral("SKU: ") + m_product.sku);
  m_nameLabel->setText(m_product.name);
  m_categoryLabel->setText(m_product.category.toUpper());

  m_stockValue->setText(QStringLiteral("%1 units").arg(m_product.currentStock));
  m_reorderValue->setText(
      QStringLiteral("%1 units").arg(m_product.reorderPoint));
  m_unitCostValue->setText(
      QStringLiteral("₹%1").arg(m_product.unitCost, 0, 'f', 2));

  // If recommendation or explanation is missing, fetch from the API.
  // We always re-fetch to get the latest enriched data (recommendation
  // was added to the API response as part of the integration fix).
  if (m_product.recommendation.isEmpty() || m_product.explanation.isEmpty()) {
    m_fetchPending = true;
    ApiClient::instance().fetchProductResult(m_product.sku);

    // ── Fallback timer ───────────────────────────────────────────────
    // If the API never responds within 10 seconds, stop showing
    // 'Loading ML Data...' and display a graceful placeholder instead.
    QTimer::singleShot(10000, this, [this]() {
      if (!m_fetchPending)
        return; // already resolved
      m_fetchPending = false;
      if (m_product.recommendation.isEmpty())
        m_product.recommendation =
            QStringLiteral("No recommendation available.");
      if (m_product.explanation.isEmpty())
        m_product.explanation =
            QStringLiteral("No detailed explanation available.");
      updateMLDetails();
    });
  }

  updateMLDetails();
  updateCharts();
  updateEOQDetails();
}

// ─────────────────────────────────────────────
// updateMLDetails — fill the prediction card
// ─────────────────────────────────────────────

void ProductDetailPanel::updateMLDetails() {
  // ── Demand badge ──
  auto dl = m_product.demandLabel;
  if (dl == DemandLabel::High) {
    m_demandBadge->setText("High Demand");
    m_demandBadge->setStyleSheet("QLabel { font-size:11px; font-weight:bold; "
                                 "background:#3d1a1a; color:#f85149;"
                                 "  border-radius:4px; padding: 3px 8px; }");
  } else if (dl == DemandLabel::Medium) {
    m_demandBadge->setText("Medium Demand");
    m_demandBadge->setStyleSheet("QLabel { font-size:11px; font-weight:bold; "
                                 "background:#3d2f0a; color:#d29922;"
                                 "  border-radius:4px; padding: 3px 8px; }");
  } else if (dl == DemandLabel::Low) {
    m_demandBadge->setText("Low Demand");
    m_demandBadge->setStyleSheet("QLabel { font-size:11px; font-weight:bold; "
                                 "background:#0d2b20; color:#3fb950;"
                                 "  border-radius:4px; padding: 3px 8px; }");
  } else {
    m_demandBadge->setText("—");
    m_demandBadge->setStyleSheet("QLabel { font-size:11px; font-weight:bold; "
                                 "background:#21262d; color:#8b949e;"
                                 "  border-radius:4px; padding: 3px 8px; }");
  }

  // ── Stock status badge ──
  auto ss = m_product.stockStatus;
  if (ss == StockStatus::Reorder) {
    m_statusBadge->setText("Reorder");
    m_statusBadge->setStyleSheet("QLabel { font-size:11px; font-weight:bold; "
                                 "background:#3d1a1a; color:#f85149;"
                                 "  border-radius:4px; padding: 3px 8px; }");
  } else if (ss == StockStatus::Overstock) {
    m_statusBadge->setText("Overstock");
    m_statusBadge->setStyleSheet("QLabel { font-size:11px; font-weight:bold; "
                                 "background:#1a2d3d; color:#58a6ff;"
                                 "  border-radius:4px; padding: 3px 8px; }");
  } else if (ss == StockStatus::NoAction) {
    m_statusBadge->setText("Healthy");
    m_statusBadge->setStyleSheet("QLabel { font-size:11px; font-weight:bold; "
                                 "background:#0d2b20; color:#3fb950;"
                                 "  border-radius:4px; padding: 3px 8px; }");
  } else {
    m_statusBadge->setText("—");
    m_statusBadge->setStyleSheet("QLabel { font-size:11px; font-weight:bold; "
                                 "background:#21262d; color:#8b949e;"
                                 "  border-radius:4px; padding: 3px 8px; }");
  }

  // ── Priority badge ──
  auto pr = m_product.priority;
  if (pr == Priority::Critical) {
    m_priorityBadge->setText("Critical");
    m_priorityBadge->setStyleSheet("QLabel { font-size:11px; font-weight:bold; "
                                   "background:#3d1a1a; color:#f85149;"
                                   "  border-radius:4px; padding: 3px 8px; }");
  } else if (pr == Priority::ReorderSoon) {
    m_priorityBadge->setText("Reorder Soon");
    m_priorityBadge->setStyleSheet("QLabel { font-size:11px; font-weight:bold; "
                                   "background:#3d2f0a; color:#d29922;"
                                   "  border-radius:4px; padding: 3px 8px; }");
  } else if (pr == Priority::Safe) {
    m_priorityBadge->setText("Safe");
    m_priorityBadge->setStyleSheet("QLabel { font-size:11px; font-weight:bold; "
                                   "background:#0d2b20; color:#3fb950;"
                                   "  border-radius:4px; padding: 3px 8px; }");
  } else {
    m_priorityBadge->setText("—");
    m_priorityBadge->setStyleSheet("QLabel { font-size:11px; font-weight:bold; "
                                   "background:#21262d; color:#8b949e;"
                                   "  border-radius:4px; padding: 3px 8px; }");
  }

  // ── Confidence ──
  if (m_product.confidence > 0.0) {
    m_confidenceVal->setText(
        QString("%1%").arg(m_product.confidence, 0, 'f', 1));
  } else {
    m_confidenceVal->setText("—");
  }

  // ── 7-day forecast ──
  if (m_product.forecastNext7 > 0.0) {
    m_forecastVal->setText(
        QString("%1 units").arg(qRound(m_product.forecastNext7)));
    m_chartSummary->setText(
        QStringLiteral("%1 units total").arg(qRound(m_product.forecastNext7)));
  } else {
    m_forecastVal->setText("—");
    m_chartSummary->setText(QStringLiteral("No forecast available"));
  }

  // ── Recommendation ────────────────────────────────────────────────────
  if (!m_product.recommendation.isEmpty()) {
    // Real data present — show it
    m_recommendationText->setText(m_product.recommendation);
    m_recommendationText->setStyleSheet(
        QStringLiteral("font-size: 12px; color: #cdd9e5; line-height: 1.5;"));
  } else if (m_fetchPending) {
    // A fetch is actively in-flight — show loading indicator
    m_recommendationText->setText(
        QStringLiteral("Fetching recommendation... ⌛"));
    m_recommendationText->setStyleSheet(
        QStringLiteral("font-size: 12px; color: #58a6ff;"));
  } else {
    // No fetch pending and no data — show graceful fallback
    m_recommendationText->setText(
        QStringLiteral("No recommendation available."));
    m_recommendationText->setStyleSheet(
        QStringLiteral("font-size: 12px; color: #6e7681;"));
  }

  // ── Explanation ───────────────────────────────────────────────────────
  if (!m_product.explanation.isEmpty()) {
    m_explanationText->setText(m_product.explanation);
    m_explanationText->setStyleSheet(
        QStringLiteral("font-size: 11px; color: #8b949e; line-height: 1.6;"));
  } else if (m_fetchPending) {
    m_explanationText->setText(QStringLiteral("Fetching explanation... ⌛"));
    m_explanationText->setStyleSheet(
        QStringLiteral("font-size: 11px; color: #58a6ff;"));
  } else {
    m_explanationText->setText(
        QStringLiteral("No detailed explanation available."));
    m_explanationText->setStyleSheet(
        QStringLiteral("font-size: 11px; color: #6e7681;"));
  }
}

// ─────────────────────────────────────────────
// updateCharts
// ─────────────────────────────────────────────

void ProductDetailPanel::updateCharts() {
  auto *chart = new QChart();
  chart->setBackgroundVisible(false);
  chart->setMargins(QMargins(5, 5, 5, 5));
  chart->layout()->setContentsMargins(0, 0, 0, 0);

  QColor textCol("#8b949e");
  QColor gridCol("#21262d");

  auto *forecastSeries = new QLineSeries();
  forecastSeries->setName("Forecast");
  QPen forecastPen;
  forecastPen.setColor(ThemeManager::instance().tokens().Accent);
  forecastPen.setWidth(2);
  forecastSeries->setPen(forecastPen);

  auto *upperSeries = new QLineSeries();
  auto *lowerSeries = new QLineSeries();

  QDateTime minDate, maxDate;
  double minY = 999999, maxY = 0;

  for (const auto &pt : m_product.forecast) {
    QDateTime dt = QDateTime(pt.date, QTime(0, 0));
    forecastSeries->append(dt.toMSecsSinceEpoch(), pt.value);
    upperSeries->append(dt.toMSecsSinceEpoch(), pt.upper);
    lowerSeries->append(dt.toMSecsSinceEpoch(), pt.lower);

    if (minDate.isNull() || dt < minDate)
      minDate = dt;
    if (maxDate.isNull() || dt > maxDate)
      maxDate = dt;
    if (pt.lower < minY)
      minY = pt.lower;
    if (pt.upper > maxY)
      maxY = pt.upper;
  }

  if (m_product.forecast.isEmpty()) {
    // Show placeholder text when no forecast data yet
    chart->setTitle(QString());
  } else {
    chart->setTitle(QString());
    auto *areaSeries = new QAreaSeries(upperSeries, lowerSeries);
    areaSeries->setName("80% Band");
    QColor bandColor(ThemeManager::instance().tokens().Accent);
    bandColor.setAlpha(35);
    areaSeries->setBrush(QBrush(bandColor));
    areaSeries->setPen(Qt::NoPen);

    chart->addSeries(areaSeries);
    chart->addSeries(forecastSeries);

    auto *axisX = new QDateTimeAxis();
    axisX->setFormat("dd MMM");
    axisX->setTitleText("Date");
    axisX->setLabelsColor(textCol);
    axisX->setTitleBrush(QBrush(textCol));
    axisX->setGridLineColor(gridCol);
    axisX->setRange(minDate, maxDate);
    axisX->setTickCount(qMin(7, m_product.forecast.size()));
    chart->addAxis(axisX, Qt::AlignBottom);
    forecastSeries->attachAxis(axisX);
    areaSeries->attachAxis(axisX);

    auto *axisY = new QValueAxis();
    axisY->setTitleText("Units");
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
  }

  auto *oldChart = m_chartView->chart();
  m_chartView->setChart(chart);
  if (oldChart)
    delete oldChart;
}

// ─────────────────────────────────────────────
// updateEOQDetails
// ─────────────────────────────────────────────

void ProductDetailPanel::updateEOQDetails() {
  m_eoqResultVal->setText(m_product.eoqQty > 0
                              ? QString("%1 units").arg(m_product.eoqQty)
                              : QString("—"));
  m_orderingCostVal->setText(
      QString("₹%1/order").arg(m_product.orderingCost, 0, 'f', 1));
  m_holdingCostVal->setText(
      QString("%1%").arg(m_product.holdingCostRate * 100, 0, 'f', 0));
  m_defaultWarning->setVisible(m_product.usingDefaultEOQ);
}

// ─────────────────────────────────────────────
// Slide animation
// ─────────────────────────────────────────────

void ProductDetailPanel::slideIn() {
  m_isSlidOut = false;
  setVisible(true);
  raise();
  adjustPanelPosition();
}

void ProductDetailPanel::slideOut() {
  m_isSlidOut = true;
  auto *parent = parentWidget();
  if (!parent)
    return;

  m_animation->stop();
  m_animation->setStartValue(pos());
  m_animation->setEndValue(QPoint(parent->width(), 0));
  connect(m_animation, &QPropertyAnimation::finished, this, [this]() {
    if (m_isSlidOut)
      setVisible(false);
  });
  m_animation->start();
}

void ProductDetailPanel::adjustPanelPosition() {
  auto *parent = parentWidget();
  if (!parent || m_isSlidOut)
    return;

  int panelW = qMin(500, parent->width() - 80);
  setFixedHeight(parent->height());
  setFixedWidth(panelW);

  m_animation->stop();
  m_animation->setStartValue(pos());
  m_animation->setEndValue(QPoint(parent->width() - panelW, 0));
  m_animation->start();
}

// ─────────────────────────────────────────────
// Events
// ─────────────────────────────────────────────

bool ProductDetailPanel::eventFilter(QObject *watched, QEvent *event) {
  if (watched == parentWidget() && event->type() == QEvent::Resize) {
    adjustPanelPosition();
  }
  return QWidget::eventFilter(watched, event);
}

void ProductDetailPanel::paintEvent(QPaintEvent * /*event*/) {
  QPainter p(this);
  p.fillRect(rect(), QColor("#0d1117"));
  p.setPen(QColor("#30363d"));
  p.drawLine(0, 0, 0, height());
}

void ProductDetailPanel::keyPressEvent(QKeyEvent *event) {
  if (event->key() == Qt::Key_Escape) {
    slideOut();
    return;
  }
  QWidget::keyPressEvent(event);
}

} // namespace Kirana
