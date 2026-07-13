#include "core/AppController.h"
#include "core/Database.h"
#include <QRandomGenerator>
#include <QtMath>
#include <algorithm>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

namespace Kirana {

// ─────────────────────────────────────────────
// Dummy product seed data
// ─────────────────────────────────────────────

struct RawSeed {
    const char* sku;
    const char* name;
    const char* category;
    int         stock;
    double      unitCost;
    DemandLabel demand;
    StockStatus status;
    Priority    priority;
    double      confidence;
    double      forecast7;  // total 7-day demand forecast (units)
    double      trend;      // units/day slope
    int         eoq;
};

static const RawSeed kSeedData[] = {
    // Critical — must restock now
    {"SKU-001","Aashirvaad Atta 5kg",         "Staples",       12,  245.0, DemandLabel::High,   StockStatus::Reorder,   Priority::Critical,    94.2,  87.5,  -2.1,  48},
    {"SKU-002","Amul Full Cream Milk 1L",      "Dairy",          8,   68.0, DemandLabel::High,   StockStatus::Reorder,   Priority::Critical,    91.5, 124.0,   3.2, 120},
    {"SKU-006","Lay's Classic Salted 26g×8",   "Snacks",         3,   20.0, DemandLabel::High,   StockStatus::Reorder,   Priority::Critical,    96.8,  95.0,   8.4, 144},
    {"SKU-012","Colgate MaxFresh 150g",         "Personal Care",  5,   78.0, DemandLabel::High,   StockStatus::Reorder,   Priority::Critical,    93.4,  55.0,   2.7,  72},
    {"SKU-014","Maggi Masala Noodles 70g×6",   "Instant Food",   0,   90.0, DemandLabel::High,   StockStatus::Reorder,   Priority::Critical,    97.2, 110.0,  12.5, 120},
    {"SKU-017","Mother Dairy Curd 500g",        "Dairy",          6,   52.0, DemandLabel::High,   StockStatus::Reorder,   Priority::Critical,    90.1,  88.0,   4.2,  90},
    {"SKU-022","Bournvita 500g",                "Beverages",      2,  245.0, DemandLabel::High,   StockStatus::Reorder,   Priority::Critical,    92.5,  72.0,   5.6,  48},
    {"SKU-025","Dabur Real Juice Apple 1L",     "Beverages",      0,  115.0, DemandLabel::High,   StockStatus::Reorder,   Priority::Critical,    94.8,  68.0,   6.3,  84},
    // Reorder Soon
    {"SKU-005","Fortune Sunflower Oil 5L",      "Oils",          22,  655.0, DemandLabel::Medium, StockStatus::NoAction,  Priority::ReorderSoon, 85.1,  43.5,   0.8,  36},
    {"SKU-009","Amul Butter 500g",              "Dairy",         14,  240.0, DemandLabel::Medium, StockStatus::Reorder,   Priority::ReorderSoon, 87.2,  32.0,   1.8,  30},
    {"SKU-016","Haldiram's Namkeen Mix 400g",   "Snacks",        18,  110.0, DemandLabel::Medium, StockStatus::NoAction,  Priority::ReorderSoon, 83.6,  28.5,   0.6,  36},
    {"SKU-020","Saffola Active Oil 2L",         "Oils",           9,  298.0, DemandLabel::Medium, StockStatus::Reorder,   Priority::ReorderSoon, 86.3,  35.5,   2.1,  24},
    // Safe / No Action
    {"SKU-003","India Gate Basmati Rice 5kg",   "Staples",       45,  380.0, DemandLabel::High,   StockStatus::NoAction,  Priority::Safe,        88.7,  62.3,   1.1,  60},
    {"SKU-007","Tata Tea Gold 500g",            "Beverages",     67,  215.0, DemandLabel::Medium, StockStatus::NoAction,  Priority::Safe,        82.4,  38.5,  -0.2,  48},
    {"SKU-008","Nescafé Classic 200g",          "Beverages",     28,  450.0, DemandLabel::Medium, StockStatus::NoAction,  Priority::Safe,        80.9,  25.0,   0.5,  24},
    {"SKU-013","Parle-G Biscuits 800g",         "Snacks",        38,   65.0, DemandLabel::High,   StockStatus::NoAction,  Priority::Safe,        89.0,  78.5,   1.4,  96},
    {"SKU-018","Kissan Mixed Fruit Jam 500g",   "Condiments",    43,  148.0, DemandLabel::Low,    StockStatus::NoAction,  Priority::Safe,        69.5,   9.5,  -0.4,  24},
    {"SKU-021","Amul Cheese Slices 200g",       "Dairy",         25,  180.0, DemandLabel::Medium, StockStatus::NoAction,  Priority::Safe,        81.7,  28.0,   0.9,  30},
    {"SKU-024","MTR Sambar Powder 100g",        "Spices",        31,   62.0, DemandLabel::Medium, StockStatus::NoAction,  Priority::Safe,        80.3,  20.0,   0.3,  48},
    // Overstock
    {"SKU-004","Tata Salt 1kg",                 "Condiments",   210,   20.0, DemandLabel::Low,    StockStatus::Overstock, Priority::Safe,        79.3,  18.0,  -0.3, 200},
    {"SKU-010","Red Label Chickpeas 1kg",        "Pulses",       180,   85.0, DemandLabel::Low,    StockStatus::Overstock, Priority::Safe,        75.6,  12.5,  -0.9,  60},
    {"SKU-011","Dettol Soap 75g×4",             "Personal Care", 55,  120.0, DemandLabel::Low,    StockStatus::NoAction,  Priority::Safe,        72.1,  10.8,   0.2,  36},
    {"SKU-015","Surf Excel Easy Wash 1kg",       "Detergent",    92,  175.0, DemandLabel::Low,    StockStatus::Overstock, Priority::Safe,        77.8,  14.0,  -1.2,  48},
    {"SKU-019","Britannia 50-50 Biscuits 400g",  "Snacks",       72,   55.0, DemandLabel::Medium, StockStatus::Overstock, Priority::Safe,        78.2,  22.0,  -0.8,  60},
    {"SKU-023","Vim Dishwash Liquid 750ml",      "Cleaning",    145,   98.0, DemandLabel::Low,    StockStatus::Overstock, Priority::Safe,        73.4,  11.5,  -1.5,  72},
};

// ─────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────

AppController::AppController(Database* db, QObject* parent)
    : QObject(parent)
    , m_db(db)
{
    qRegisterMetaType<Kirana::Product>();
    qRegisterMetaType<QVector<Kirana::Product>>();
}

// ─────────────────────────────────────────────
// loadDummyData
// ─────────────────────────────────────────────

void AppController::loadDummyData() {
    m_products     = buildDummyProducts(m_settings);
    m_lastRunTime  = QDateTime::currentDateTime();
    m_pipelineLive = false;

    emit productsChanged(m_products);
    emit pipelineStateChanged(m_pipelineLive, m_lastRunTime);
}

// ─────────────────────────────────────────────
// loadFromDatabase
// ─────────────────────────────────────────────

bool AppController::loadFromDatabase() {
    if (!m_db) return false;
    QVector<Product> dbProds = m_db->getProducts();
    if (dbProds.isEmpty()) {
        // Seed database with dummy seed data
        QVector<Product> dummyList = buildDummyProducts(m_settings);
        for (const auto& p : dummyList) {
            int newId = m_db->saveProduct(p);
            if (newId > 0) {
                // Seed 30 days of sales history daily entries
                QDate today = QDate::currentDate();
                for (int d = 30; d >= 1; --d) {
                    DailyEntry entry;
                    entry.productId = newId;
                    entry.entryDate = today.addDays(-d);
                    entry.unitsSold = p.historicalSales[30 - d];
                    entry.unitsWasted = QRandomGenerator::global()->bounded(2);
                    m_db->saveDailyEntry(entry);
                }
            }
        }
        dbProds = m_db->getProducts();
    }

    // Try loading actual ML pipeline results from Database
    QString jsonStr = m_db->getLatestPipelineResults();
    bool loadedMLResults = false;
    if (!jsonStr.isEmpty()) {
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &err);
        if (err.error == QJsonParseError::NoError && doc.isArray()) {
            QJsonArray arr = doc.array();
            for (const QJsonValue& val : arr) {
                QJsonObject obj = val.toObject();
                QString sku = obj.value(QStringLiteral("sku")).toString();
                for (auto& p : dbProds) {
                    if (p.sku == sku) {
                        QString dl = obj.value(QStringLiteral("demand_label")).toString();
                        if (dl == QStringLiteral("High") || dl == QStringLiteral("High Demand")) p.demandLabel = DemandLabel::High;
                        else if (dl == QStringLiteral("Medium") || dl == QStringLiteral("Medium Demand")) p.demandLabel = DemandLabel::Medium;
                        else p.demandLabel = DemandLabel::Low;

                        QString ss = obj.value(QStringLiteral("stock_status")).toString();
                        if (ss == QStringLiteral("Reorder")) p.stockStatus = StockStatus::Reorder;
                        else if (ss == QStringLiteral("Overstock")) p.stockStatus = StockStatus::Overstock;
                        else p.stockStatus = StockStatus::NoAction;

                        QString pr = obj.value(QStringLiteral("priority")).toString();
                        if (pr == QStringLiteral("Critical")) p.priority = Priority::Critical;
                        else if (pr == QStringLiteral("ReorderSoon") || pr == QStringLiteral("Reorder Soon")) p.priority = Priority::ReorderSoon;
                        else p.priority = Priority::Safe;

                        p.confidence = obj.value(QStringLiteral("confidence")).toDouble();
                        p.forecastNext7 = obj.value(QStringLiteral("forecast_7d")).toDouble();
                        p.forecastTrend = obj.value(QStringLiteral("trend")).toDouble();
                        p.eoqQty = obj.value(QStringLiteral("eoq")).toInt();

                        p.forecast.clear();
                        QJsonArray pts = obj.value(QStringLiteral("forecast_points")).toArray();
                        for (const QJsonValue& ptVal : pts) {
                            QJsonObject ptObj = ptVal.toObject();
                            ForecastPoint fp;
                            fp.date = QDate::fromString(ptObj.value(QStringLiteral("ds")).toString(), QStringLiteral("yyyy-MM-dd"));
                            fp.value = ptObj.value(QStringLiteral("yhat")).toDouble();
                            fp.lower = ptObj.value(QStringLiteral("yhat_lower")).toDouble();
                            fp.upper = ptObj.value(QStringLiteral("yhat_upper")).toDouble();
                            p.forecast.append(fp);
                        }

                        const double dailyBase = p.forecastNext7 / 7.0;
                        p.historicalSales = makeHistory(dailyBase, 30);
                        break;
                    }
                }
            }
            m_pipelineLive = true;
            loadedMLResults = true;
        }
    }

    if (!loadedMLResults) {
        // Populate ML simulation values on top of DB loaded structures
        for (int i = 0; i < dbProds.size(); ++i) {
            Product& p = dbProds[i];
            bool foundSeed = false;
            for (const auto& s : kSeedData) {
                if (p.sku == QString::fromLatin1(s.sku)) {
                    p.demandLabel = s.demand;
                    p.stockStatus = s.status;
                    p.priority = s.priority;
                    p.confidence = s.confidence;
                    p.forecastNext7 = s.forecast7;
                    p.forecastTrend = s.trend;
                    p.eoqQty = s.eoq;
                    foundSeed = true;
                    break;
                }
            }
            if (!foundSeed) {
                // Default generated ML/forecasting values
                p.demandLabel = DemandLabel::Medium;
                p.stockStatus = StockStatus::NoAction;
                p.priority = Priority::Safe;
                p.confidence = 85.0;
                p.forecastNext7 = p.currentStock * 0.5;
                p.forecastTrend = 0.5;
                p.eoqQty = 50;
            }

            const double dailyBase = p.forecastNext7 / 7.0;
            p.forecast = makeForecast(dailyBase, p.forecastTrend / 7.0, m_settings.forecastHorizonDays);
            p.historicalSales = makeHistory(dailyBase, 30);
        }
    }

    m_products = dbProds;
    m_lastRunTime  = QDateTime::currentDateTime();
    emit productsChanged(m_products);
    return true;
}

// ─────────────────────────────────────────────
// applyPipelineResults  (called from Phase 2 worker)
// ─────────────────────────────────────────────

void AppController::applyPipelineResults(QVector<Product> updated) {
    m_products     = std::move(updated);
    m_lastRunTime  = QDateTime::currentDateTime();
    m_pipelineLive = true;

    emit productsChanged(m_products);
    emit pipelineStateChanged(m_pipelineLive, m_lastRunTime);
}

void AppController::applyPipelineRun(const PipelineRunResult& result) {
    QVector<Product> updatedList = m_products;
    for (const auto& r : result.results) {
        for (auto& p : updatedList) {
            if ((r.productId > 0 && p.id == r.productId) || (p.sku == r.sku)) {
                p.demandLabel   = r.demandLabel;
                p.stockStatus   = r.stockStatus;
                p.confidence    = r.confidence;
                p.priority      = r.priority;
                p.forecastNext7 = r.forecastNext7;
                p.forecastTrend = r.forecastTrend;
                p.forecast      = r.forecast;
                p.eoqQty        = r.eoqQty;

                const double dailyBase = p.forecastNext7 / 7.0;
                p.historicalSales = makeHistory(dailyBase, 30);
                break;
            }
        }
    }
    applyPipelineResults(std::move(updatedList));
}

void AppController::updateSettings(const AppSettings& s) {
    m_settings = s;
    emit settingsChanged(m_settings);
}

// ─────────────────────────────────────────────
// Aggregate statistics
// ─────────────────────────────────────────────

int AppController::criticalCount() const {
    return static_cast<int>(
        std::count_if(m_products.begin(), m_products.end(),
            [](const Product& p) { return p.priority == Priority::Critical; }));
}

int AppController::overstockCount() const {
    return static_cast<int>(
        std::count_if(m_products.begin(), m_products.end(),
            [](const Product& p) { return p.stockStatus == StockStatus::Overstock; }));
}

double AppController::stockoutRiskPct() const {
    if (m_products.isEmpty()) return 0.0;
    const int atRisk = static_cast<int>(
        std::count_if(m_products.begin(), m_products.end(),
            [](const Product& p) {
                return p.stockStatus == StockStatus::Reorder ||
                       p.currentStock == 0;
            }));
    return (atRisk * 100.0) / m_products.size();
}

// ─────────────────────────────────────────────
// Static helpers
// ─────────────────────────────────────────────

QVector<ForecastPoint> AppController::makeForecast(double base, double trend, int days) {
    QVector<ForecastPoint> pts;
    const QDate today = QDate::currentDate();
    auto* rng = QRandomGenerator::global();

    for (int d = 0; d < days; ++d) {
        const double mu     = base + trend * d;
        const double noise  = (rng->bounded(200) / 100.0 - 1.0) * base * 0.15;
        const double val    = qMax(0.0, mu + noise);
        ForecastPoint fp;
        fp.date  = today.addDays(d + 1);
        fp.value = val;
        fp.lower = qMax(0.0, val * 0.75);
        fp.upper = val * 1.30;
        pts.append(fp);
    }
    return pts;
}

QVector<double> AppController::makeHistory(double base, int days) {
    QVector<double> hist;
    auto* rng = QRandomGenerator::global();
    for (int d = 0; d < days; ++d) {
        const double noise = (rng->bounded(200) / 100.0 - 1.0) * base * 0.25;
        const double wave  = base * 0.12 * qSin(2.0 * M_PI * d / 7.0);
        hist.append(qMax(0.0, base + wave + noise));
    }
    return hist;
}

QVector<Product> AppController::buildDummyProducts(const AppSettings& cfg) {
    QVector<Product> result;
    int id = 1;

    for (const auto& s : kSeedData) {
        Product p;
        p.id             = id++;
        p.sku            = QString::fromLatin1(s.sku);
        p.name           = QString::fromUtf8(s.name);
        p.category       = QString::fromLatin1(s.category);
        p.currentStock   = s.stock;
        p.unitCost       = s.unitCost;
        p.demandLabel    = s.demand;
        p.stockStatus    = s.status;
        p.priority       = s.priority;
        p.confidence     = s.confidence;
        p.forecastNext7  = s.forecast7;
        p.forecastTrend  = s.trend;
        p.eoqQty         = s.eoq;
        p.orderingCost   = cfg.orderingCost;
        p.holdingCostRate= cfg.holdingCostRate;
        p.usingDefaultEOQ= true;

        const double dailyBase = s.forecast7 / 7.0;
        p.forecast       = makeForecast(dailyBase, s.trend / 7.0, cfg.forecastHorizonDays);
        p.historicalSales= makeHistory(dailyBase, 30);

        result.append(p);
    }

    return result;
}

} // namespace Kirana
