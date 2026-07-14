#include "core/ThemeManager.h"
#include "ui/DashboardWidget.h"
#include "core/AppController.h"
#include "core/Database.h"
#include "core/ProductModel.h"
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
#include <QPainter>
#include <QPaintEvent>
#include <QPainterPath>
#include <QScrollArea>
#include <QLinearGradient>
#include <cmath>

namespace Kirana {

// ══════════════════════════════════════════════
// ProductFilterProxyModel
// ══════════════════════════════════════════════

class ProductFilterProxyModel : public QSortFilterProxyModel {
public:
    explicit ProductFilterProxyModel(QObject* parent = nullptr)
        : QSortFilterProxyModel(parent)
    {}

    QString filterState = QStringLiteral("All");

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override {
        if (filterState.isEmpty() || filterState == QLatin1String("All"))
            return true;

        auto* model = qobject_cast<ProductModel*>(sourceModel());
        if (!model) return true;

        const Product& p = model->productAt(sourceRow);

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
        const auto& tokens = ThemeManager::instance().tokens();
        struct Seg { double pct; QColor col; };
        Seg segs[] = {
            { m_safe,      tokens.Success },
            { m_overstock, tokens.Warning },
            { m_reorder,   tokens.Critical },
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
    const auto& tokens = ThemeManager::instance().tokens();
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 20, 24, 20);
    mainLayout->setSpacing(16);

    // ── Page header ─────────────────────────────
    auto* headerRow = new QHBoxLayout;
    auto* pageTitle = new QLabel(QStringLiteral("Dashboard"), this);
    pageTitle->setObjectName("HeadlineLg");
    pageTitle->setStyleSheet(
        "font-family:'SF Mono','Menlo','Cascadia Mono','Consolas',monospace;"
        "font-size:24px;font-weight:bold;color:#e5e5e5;"
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

    m_cardSKUs     = new MetricCard(QStringLiteral("Total SKUs"),      tokens.Success, this);
    m_cardCritical = new MetricCard(QStringLiteral("Critical Reorder"), tokens.Critical, this);
    m_cardOverstock= new MetricCard(QStringLiteral("Overstock"),        tokens.Warning, this);
    m_cardStockout = new MetricCard(QStringLiteral("Stockout Risk"),     tokens.Accent, this);

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
    ThemeManager::applyDropShadow(queueCard, 20, QColor(tokens.Accent.red(), tokens.Accent.green(), tokens.Accent.blue(), 30));
    queueCard->setMinimumHeight(480);
    auto* queueLayout = new QVBoxLayout(queueCard);
    queueLayout->setContentsMargins(0, 0, 0, 0);
    queueLayout->setSpacing(0);

    // Queue header
    auto* queueHeader = new QWidget(queueCard);
    queueHeader->setStyleSheet(
        "background: rgba(10,10,10,0.5);"
        "border-bottom: 1px solid #222222;");
    queueHeader->setFixedHeight(52);
    auto* queueHRow = new QHBoxLayout(queueHeader);
    queueHRow->setContentsMargins(20, 0, 16, 0);

    auto* queueTitle = new QLabel(QStringLiteral("Priority Queue"), queueHeader);
    queueTitle->setStyleSheet(
        "font-family:'SF Mono','Menlo','Cascadia Mono','Consolas',monospace;"
        "font-size:14px;font-weight:bold;color:#e5e5e5;"
        "background:transparent;border:none;");

    auto* viewAllQ = new QPushButton(QStringLiteral("View All"), queueHeader);
    viewAllQ->setObjectName("SecondaryBtn");
    viewAllQ->setFixedHeight(28);
    viewAllQ->setCursor(Qt::PointingHandCursor);
    viewAllQ->setStyleSheet(
        "QPushButton{background:transparent;border:none;"
        "color:#d97706;font-family:'SF Mono','Menlo','Cascadia Mono','Consolas',monospace;"
        "font-size:13px;font-weight:bold;padding:0;}"
        "QPushButton:hover{color:#f59e0b;}");

    queueHRow->addWidget(queueTitle);
    queueHRow->addStretch();
    queueHRow->addWidget(viewAllQ);

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
    m_tableView->setStyleSheet("QTableView { border: none; background: transparent; }");

    // Filter chips row
    auto* chipsRow = new QWidget(queueCard);
    chipsRow->setStyleSheet("background: transparent; border-bottom: 1px solid #222222;");
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
    ThemeManager::applyDropShadow(donutCard, 20, QColor(tokens.Accent.red(), tokens.Accent.green(), tokens.Accent.blue(), 30));
    donutCard->setMinimumHeight(480);
    auto* donutLayout = new QVBoxLayout(donutCard);
    donutLayout->setContentsMargins(20, 20, 20, 20);
    donutLayout->setSpacing(16);

    auto* donutTitle = new QLabel(QStringLiteral("Stock Distribution"), donutCard);
    donutTitle->setStyleSheet(
        "font-family:'SF Mono','Menlo','Cascadia Mono','Consolas',monospace;"
        "font-size:14px;font-weight:bold;color:#e5e5e5;"
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

    m_legendSafe      = makeLegendRow(QStringLiteral("Safe Stock"), tokens.Success);
    m_legendOverstock = makeLegendRow(QStringLiteral("Overstock"),  tokens.Warning);
    m_legendReorder   = makeLegendRow(QStringLiteral("Reorder"),    tokens.Critical);

    lowerRow->addWidget(queueCard, 2);
    lowerRow->addWidget(donutCard, 1);
    mainLayout->addLayout(lowerRow, 1);

    // Detail panel (hidden overlay)
    m_detailPanel = new ProductDetailPanel(this);
    m_detailPanel->setVisible(false);
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

    auto* filterProxy = new ProductFilterProxyModel(this);
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
        proxy->invalidate();
    }
}

void DashboardWidget::onRowDoubleClicked(const QModelIndex& index) {
    if (!index.isValid()) return;
    QModelIndex sourceIdx = m_proxyModel->mapToSource(index);
    const Product& product = m_model->productAt(sourceIdx.row());
    m_detailPanel->setProduct(product);
    m_detailPanel->slideIn();
}

void DashboardWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    if (m_detailPanel && m_detailPanel->isVisible())
        m_detailPanel->adjustPanelPosition();
}

} // namespace Kirana
