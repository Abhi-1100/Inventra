#include "core/ThemeManager.h"
#include "ui/DashboardWidget.h"
#include "core/AppController.h"
#include "core/Database.h"
#include "core/ProductModel.h"
#include "core/ApiClient.h"          // ---- ADDED: API Integration ----
#include "widgets/MetricCard.h"
#include "widgets/BadgeDelegate.h"
#include "ui/ProductDetailPanel.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableView>
#include <QHeaderView>
#include <QSortFilterProxyModel>
#include <QResizeEvent>
#include <QPushButton>
#include <QListWidget>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPaintEvent>
#include <QPainterPath>
#include <QScrollArea>
#include <QLinearGradient>
#include <QTimer>                    // ---- ADDED: API Integration ----
#include <cmath>

namespace Kirana {

// ══════════════════════════════════════════════
// ProductFilterProxyModel
// ══════════════════════════════════════════════

class ProductFilterProxyModel : public QSortFilterProxyModel {
public:
    explicit ProductFilterProxyModel(AppController* ctrl, QObject* parent = nullptr)
        : QSortFilterProxyModel(parent), controller(ctrl)
    {}

    AppController* controller;
    QString filterState = QStringLiteral("All");
    QString searchQuery;

    void refreshFilter() {
        beginResetModel();
        invalidateFilter();
        endResetModel();
    }

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override {
        auto* model = qobject_cast<ProductModel*>(sourceModel());
        if (!model) return true;

        const Product& p = model->productAt(sourceRow);

        if (controller && !controller->matchesSearch(p, searchQuery)) {
            return false;
        }

        if (filterState.isEmpty() || filterState == QLatin1String("All"))
            return true;

        if (filterState == QLatin1String("Critical"))
            return p.priority == Priority::Critical;
        if (filterState == QLatin1String("Reorder"))
            return p.stockStatus == StockStatus::Reorder || p.priority == Priority::ReorderSoon;
        if (filterState == QLatin1String("Overstock"))
            return p.stockStatus == StockStatus::Overstock;
        if (filterState == QLatin1String("Safe"))
            return p.priority == Priority::Safe;

        return true;
    }
};

// ══════════════════════════════════════════════
// StockDonutWidget — conic gradient donut
// ══════════════════════════════════════════════

class StockDonutWidget : public QWidget {
public:
    explicit StockDonutWidget(QWidget* parent = nullptr)
        : QWidget(parent) {
        setMinimumSize(160, 160);
    }

    void setData(double safe, double overstock, double reorder) {
        m_safe = safe; m_overstock = overstock; m_reorder = reorder;
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        const QRect r = rect();
        const int  sz = qMin(r.width(), r.height()) - 20;
        const QRect arc(r.center().x() - sz/2, r.center().y() - sz/2, sz, sz);
        const int thick = sz / 5;

        // Segments: safe=primary, overstock=tertiary, reorder=error
        struct Seg { double pct; QColor col; };
        Seg segs[] = {
            { m_safe,      QColor("#afc6ff") },
            { m_overstock, QColor("#c0c7d3") },
            { m_reorder,   QColor("#ffb4ab") },
        };

        int startAngle = 90 * 16; // 12 o'clock
        for (auto& seg : segs) {
            if (seg.pct <= 0) continue;
            int span = qRound(seg.pct * 360.0 * 16.0);
            p.setPen(Qt::NoPen);
            // Draw arc segment using path
            QPainterPath outer, inner, path;
            outer.arcTo(arc, startAngle / 16.0, -span / 16.0);
            QRect innerArc = arc.adjusted(thick, thick, -thick, -thick);
            inner.arcTo(innerArc, startAngle / 16.0 - span / 16.0, span / 16.0);

            // Use drawArc for simplicity
            QPen pen(seg.col, thick, Qt::SolidLine, Qt::FlatCap);
            p.setPen(pen);
            p.setBrush(Qt::NoBrush);
            p.drawArc(arc.adjusted(thick/2, thick/2, -thick/2, -thick/2),
                      startAngle, -span);
            startAngle -= span;
        }

        // Center text
        p.setPen(QColor("#e0e2ea"));
        QFont f;
        f.setFamily("JetBrains Mono");
        f.setPixelSize(18);
        f.setWeight(QFont::Medium);
        p.setFont(f);
        p.drawText(arc, Qt::AlignCenter, QStringLiteral("Safe\n") +
                   QString::number(qRound(m_safe * 100)) + QStringLiteral("%"));
    }

private:
    double m_safe = 0.65, m_overstock = 0.20, m_reorder = 0.15;
};

// ══════════════════════════════════════════════
// DashboardWidget
// ══════════════════════════════════════════════

DashboardWidget::DashboardWidget(AppController* controller, QWidget* parent)
    : QWidget(parent)
    , m_controller(controller)
{
    buildLayout();
    configureTable();
    connect(m_controller, &AppController::productsChanged, this, &DashboardWidget::onProductsChanged);
    onProductsChanged();
}

void DashboardWidget::buildLayout() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 20, 24, 20);
    mainLayout->setSpacing(16);

    // ── Page header ─────────────────────────────
    auto* headerRow = new QHBoxLayout;
    auto* pageTitle = new QLabel(QStringLiteral("Dashboard"), this);
    pageTitle->setObjectName("HeadlineLg");
    pageTitle->setStyleSheet(
        "font-family:'Hanken Grotesk','Segoe UI',sans-serif;"
        "font-size:28px;font-weight:700;color:#e0e2ea;"
        "background:transparent;border:none;");
    headerRow->addWidget(pageTitle);
    headerRow->addStretch();

    // "View All" button
    auto* viewAllBtn = new QPushButton(QStringLiteral("View All"), this);
    viewAllBtn->setObjectName("SecondaryBtn");
    viewAllBtn->setFixedHeight(32);
    viewAllBtn->setCursor(Qt::PointingHandCursor);
    headerRow->addWidget(viewAllBtn);
    mainLayout->addLayout(headerRow);

    // ── Metric Cards Row ─────────────────────────
    auto* cardsLayout = new QHBoxLayout;
    cardsLayout->setSpacing(16);

    m_cardSKUs     = new MetricCard(QStringLiteral("Total SKUs"),      QColor("#afc6ff"), this);
    m_cardCritical = new MetricCard(QStringLiteral("Critical Reorder"), QColor("#ffb4ab"), this);
    m_cardOverstock= new MetricCard(QStringLiteral("Overstock"),        QColor("#c0c7d3"), this);
    m_cardStockout = new MetricCard(QStringLiteral("Stockout Risk"),     QColor("#acc7ff"), this);

    // Set icon characters for each card
    m_cardSKUs->setIconText     (QStringLiteral("📦"));
    m_cardCritical->setIconText (QStringLiteral("⚠"));
    m_cardOverstock->setIconText(QStringLiteral("≡"));
    m_cardStockout->setIconText (QStringLiteral("↓"));

    cardsLayout->addWidget(m_cardSKUs);
    cardsLayout->addWidget(m_cardCritical);
    cardsLayout->addWidget(m_cardOverstock);
    cardsLayout->addWidget(m_cardStockout);
    mainLayout->addLayout(cardsLayout);

    // ── Lower 2/3 + 1/3 split ────────────────────
    auto* lowerRow = new QHBoxLayout;
    lowerRow->setSpacing(16);

    // ── Priority Queue (2/3) ─────────────────────
    auto* queueCard = new QFrame(this);
    queueCard->setObjectName("GlassCard");
    queueCard->setMinimumHeight(480);
    auto* queueLayout = new QVBoxLayout(queueCard);
    queueLayout->setContentsMargins(0, 0, 0, 0);
    queueLayout->setSpacing(0);

    // Queue header
    auto* queueHeader = new QWidget(queueCard);
    queueHeader->setStyleSheet(
        "background: rgba(11,15,20,0.5);"
        "border-bottom: 1px solid #232a33;");
    queueHeader->setFixedHeight(52);
    auto* queueHRow = new QHBoxLayout(queueHeader);
    queueHRow->setContentsMargins(20, 0, 16, 0);

    auto* queueTitle = new QLabel(QStringLiteral("Priority Queue"), queueHeader);
    queueTitle->setStyleSheet(
        "font-family:'Hanken Grotesk',sans-serif;"
        "font-size:16px;font-weight:600;color:#e0e2ea;"
        "background:transparent;border:none;");

    m_searchEdit = new QLineEdit(queueHeader);
    m_searchEdit->setPlaceholderText(QStringLiteral("Search products..."));
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setFixedSize(210, 30);
    m_searchEdit->setStyleSheet(
        "QLineEdit { background:#0a0e13; border:1px solid #232a33; border-radius:5px;"
        "color:#e0e2ea; font-size:12px; padding:0 10px; }"
        "QLineEdit:focus { border-color:#afc6ff; }"
        "QLineEdit::placeholder { color:rgba(140,144,160,0.65); }");

    auto* viewAllQ = new QPushButton(QStringLiteral("View All"), queueHeader);
    viewAllQ->setObjectName("SecondaryBtn");
    viewAllQ->setFixedHeight(28);
    viewAllQ->setCursor(Qt::PointingHandCursor);
    viewAllQ->setStyleSheet(
        "QPushButton{background:transparent;border:none;"
        "color:#afc6ff;font-family:'Hanken Grotesk',sans-serif;"
        "font-size:13px;font-weight:600;padding:0;}"
        "QPushButton:hover{color:#d9e2ff;}");

    queueHRow->addWidget(queueTitle);
    queueHRow->addStretch();
    queueHRow->addWidget(m_searchEdit);
    queueHRow->addSpacing(12);
    queueHRow->addWidget(viewAllQ);

    connect(m_searchEdit, &QLineEdit::textChanged, this, [this](const QString& query) {
        if (auto* proxy = static_cast<ProductFilterProxyModel*>(m_proxyModel)) {
            proxy->searchQuery = query.trimmed();
            proxy->refreshFilter();
        }
    });

    // Table view
    m_tableView = new QTableView(queueCard);
    m_tableView->setObjectName("ProductTableView");
    m_tableView->setSortingEnabled(true);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->setShowGrid(false);
    m_tableView->setAlternatingRowColors(false);
    m_tableView->verticalHeader()->setVisible(false);
    m_tableView->verticalHeader()->setDefaultSectionSize(48);
    m_tableView->setFrameShape(QFrame::NoFrame);
    m_tableView->setStyleSheet(
        "QTableView {"
        "  background: transparent;"
        "  border: none;"
        "  border-radius: 0;"
        "  gridline-color: transparent;"
        "  selection-background-color: #1c2025;"
        "}"
        "QTableView::item {"
        "  border-bottom: 1px solid rgba(35,42,51,0.5);"
        "  padding: 0 16px;"
        "  background: transparent;"
        "}"
        "QTableView::item:hover { background: #1c2025; }"
        "QTableView::item:selected { background: #1c2025; color: #e0e2ea; }"
        "QHeaderView::section {"
        "  background: #0a0e13;"
        "  color: #8c90a0;"
        "  font-family:'Hanken Grotesk',sans-serif;"
        "  font-size:13px;font-weight:600;letter-spacing:0.04em;"
        "  padding: 10px 16px;"
        "  border: none;"
        "  border-bottom: 1px solid #232a33;"
        "}");

    // Filter chips row
    auto* chipsRow = new QWidget(queueCard);
    chipsRow->setStyleSheet("background: transparent; border-bottom: 1px solid #232a33;");
    chipsRow->setFixedHeight(48);
    auto* chipsLayout = new QHBoxLayout(chipsRow);
    chipsLayout->setContentsMargins(16, 8, 16, 8);
    chipsLayout->setSpacing(8);
    setupFilterChips(chipsLayout);
    chipsLayout->addStretch();

    queueLayout->addWidget(queueHeader);
    queueLayout->addWidget(chipsRow);
    queueLayout->addWidget(m_tableView, 1);

    // ── Stock Distribution (1/3) ──────────────────
    auto* donutCard = new QFrame(this);
    donutCard->setObjectName("GlassCard");
    donutCard->setMinimumHeight(480);
    auto* donutLayout = new QVBoxLayout(donutCard);
    donutLayout->setContentsMargins(20, 20, 20, 20);
    donutLayout->setSpacing(16);

    auto* donutTitle = new QLabel(QStringLiteral("Stock Distribution"), donutCard);
    donutTitle->setStyleSheet(
        "font-family:'Hanken Grotesk',sans-serif;"
        "font-size:16px;font-weight:600;color:#e0e2ea;"
        "background:transparent;border:none;");
    donutLayout->addWidget(donutTitle);

    m_donutWidget = new StockDonutWidget(donutCard);
    donutLayout->addWidget(m_donutWidget, 1, Qt::AlignCenter);

    // Legend
    auto makeLegendRow = [&](const QString& label, const QColor& col) {
        auto* row = new QHBoxLayout;
        row->setSpacing(8);
        auto* dot = new QLabel(donutCard);
        dot->setFixedSize(12, 12);
        dot->setStyleSheet(QString(
            "background:%1;border-radius:2px;border:none;").arg(col.name()));
        auto* lbl = new QLabel(label, donutCard);
        lbl->setStyleSheet("font-size:13px;color:#c2c6d6;background:transparent;border:none;");
        auto* val = new QLabel(donutCard);
        val->setStyleSheet("font-family:'JetBrains Mono',monospace;"
                           "font-size:13px;color:#e0e2ea;background:transparent;border:none;");
        val->setObjectName(label + QStringLiteral("_val"));
        row->addWidget(dot);
        row->addWidget(lbl);
        row->addStretch();
        row->addWidget(val);
        donutLayout->addLayout(row);
        return val;
    };

    m_legendSafe      = makeLegendRow(QStringLiteral("Safe Stock"), QColor("#afc6ff"));
    m_legendOverstock = makeLegendRow(QStringLiteral("Overstock"),  QColor("#c0c7d3"));
    m_legendReorder   = makeLegendRow(QStringLiteral("Reorder"),    QColor("#ffb4ab"));

    lowerRow->addWidget(queueCard, 2);
    lowerRow->addWidget(donutCard, 1);
    mainLayout->addLayout(lowerRow, 1);

    // Detail panel (hidden overlay)
    m_detailPanel = new ProductDetailPanel(this);
    m_detailPanel->setVisible(false);

    // Forward single-product prediction request up to MainWindow
    connect(m_detailPanel, &ProductDetailPanel::runPredictionRequested,
            this, &DashboardWidget::singleProductPredictionRequested);
}

void DashboardWidget::setupFilterChips(QHBoxLayout* rowLayout) {
    struct ChipInfo { QString label; QString value; };
    QVector<ChipInfo> chips = {
        { QStringLiteral("All Products"), QStringLiteral("All") },
        { QStringLiteral("Critical"),     QStringLiteral("Critical") },
        { QStringLiteral("Reorder"),      QStringLiteral("Reorder") },
        { QStringLiteral("Overstock"),    QStringLiteral("Overstock") },
        { QStringLiteral("Safe"),         QStringLiteral("Safe") },
    };

    for (const auto& chip : chips) {
        auto* btn = new QPushButton(chip.label, this);
        btn->setObjectName(QStringLiteral("FilterChip"));
        btn->setCursor(Qt::PointingHandCursor);
        btn->setProperty("value", chip.value);
        btn->setProperty("active", chip.value == QLatin1String("All"));
        connect(btn, &QPushButton::clicked, this, &DashboardWidget::onFilterChipClicked);
        m_filterChips.append(btn);
        rowLayout->addWidget(btn);
    }
}

void DashboardWidget::configureTable() {
    m_model = new ProductModel(this);

    auto* filterProxy = new ProductFilterProxyModel(m_controller, this);
    filterProxy->setSourceModel(m_model);
    filterProxy->setSortRole(Qt::DisplayRole);
    m_proxyModel = filterProxy;

    m_tableView->setModel(m_proxyModel);

    auto* badgeDelegate    = new BadgeDelegate(BadgeDelegate::Mode::Badge,    m_tableView);
    auto* forecastDelegate = new BadgeDelegate(BadgeDelegate::Mode::Forecast, m_tableView);

    m_tableView->setItemDelegateForColumn(static_cast<int>(ProductColumn::DemandLabel), badgeDelegate);
    m_tableView->setItemDelegateForColumn(static_cast<int>(ProductColumn::StockStatus), badgeDelegate);
    m_tableView->setItemDelegateForColumn(static_cast<int>(ProductColumn::Priority),    badgeDelegate);
    m_tableView->setItemDelegateForColumn(static_cast<int>(ProductColumn::Forecast),    forecastDelegate);

    QHeaderView* header = m_tableView->horizontalHeader();
    header->setSectionResizeMode(QHeaderView::Interactive);
    header->setStretchLastSection(true);
    header->setHighlightSections(false);

    m_tableView->setColumnWidth(static_cast<int>(ProductColumn::Name),        220);
    m_tableView->setColumnWidth(static_cast<int>(ProductColumn::DemandLabel),  90);
    m_tableView->setColumnWidth(static_cast<int>(ProductColumn::StockStatus), 110);
    m_tableView->setColumnWidth(static_cast<int>(ProductColumn::Confidence),   90);
    m_tableView->setColumnWidth(static_cast<int>(ProductColumn::Forecast),    100);
    m_tableView->setColumnWidth(static_cast<int>(ProductColumn::Priority),    110);
    m_tableView->setColumnWidth(static_cast<int>(ProductColumn::EOQQty),       85);

    connect(m_tableView, &QTableView::doubleClicked, this, &DashboardWidget::onRowDoubleClicked);
}

void DashboardWidget::updateMetrics() {
    const int total    = m_controller->products().size();
    const int critical = m_controller->criticalCount();
    const int overstock= m_controller->overstockCount();
    const double risk  = m_controller->stockoutRiskPct();

    m_cardSKUs->setValue(QStringLiteral("%L1").arg(total));
    m_cardSKUs->setSubtitle(QStringLiteral("Total active items"));

    m_cardCritical->setValue(QString::number(critical));
    m_cardCritical->setSubtitle(QStringLiteral("Immediate orders needed"));

    m_cardOverstock->setValue(QString::number(overstock));
    m_cardOverstock->setSubtitle(QStringLiteral("Excess capital items"));

    m_cardStockout->setValue(QStringLiteral("%1%").arg(risk, 0, 'f', 1));
    m_cardStockout->setSubtitle(QStringLiteral("Products at stockout risk"));

    // Update donut percentages
    const double safeCount = qMax(0, total - critical - overstock);
    if (total > 0 && m_donutWidget) {
        m_donutWidget->setData(
            safeCount   / total,
            (double)overstock / total,
            (double)critical  / total);
    }
    if (m_legendSafe)      m_legendSafe->setText(QString::number(qRound(100.0*safeCount/qMax(1,total))) + "%");
    if (m_legendOverstock) m_legendOverstock->setText(QString::number(qRound(100.0*overstock/qMax(1,total))) + "%");
    if (m_legendReorder)   m_legendReorder->setText(QString::number(qRound(100.0*critical/qMax(1,total))) + "%");
}

void DashboardWidget::updateActivityFeed() {
    // Activity feed is now part of the detail panel; left panel is the priority table
}

void DashboardWidget::onProductsChanged() {
    m_model->setProducts(m_controller->products());
    updateMetrics();
}

void DashboardWidget::onFilterChipClicked() {
    auto* clicked = qobject_cast<QPushButton*>(sender());
    if (!clicked) return;

    QString val = clicked->property("value").toString();
    for (auto* btn : m_filterChips) {
        btn->setProperty("active", btn == clicked);
        btn->style()->unpolish(btn);
        btn->style()->polish(btn);
        btn->update();
    }

    auto* proxy = static_cast<ProductFilterProxyModel*>(m_proxyModel);
    if (proxy) {
        proxy->filterState = val;
        proxy->refreshFilter();
    }
}

void DashboardWidget::onRowDoubleClicked(const QModelIndex& index) {
    if (!index.isValid()) return;
    QModelIndex sourceIdx = m_proxyModel->mapToSource(index);
    const Product& product = m_model->productAt(sourceIdx.row());
    m_detailPanel->setProduct(product);
    m_detailPanel->slideIn();
}

void DashboardWidget::onSingleProductPredictionFinished(int productId) {
    // Find the updated product in the controller and refresh the panel
    const auto& products = m_controller->products();
    for (const auto& p : products) {
        if (p.id == productId) {
            m_detailPanel->setProduct(p);
            break;
        }
    }
    // Make sure the panel is visible with updated data
    if (!m_detailPanel->isVisible()) {
        m_detailPanel->slideIn();
    }
}

void DashboardWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    if (m_detailPanel && m_detailPanel->isVisible())
        m_detailPanel->adjustPanelPosition();
}

// API methods removed, now relying on AppController single source of truth

} // namespace Kirana
