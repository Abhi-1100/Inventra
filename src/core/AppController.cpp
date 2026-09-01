#include "core/AppController.h"
#include "core/Database.h"
#include "core/ApiClient.h"
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
    
    // Connect to ApiClient to act as single source of truth
    connect(&ApiClient::instance(), &ApiClient::resultsReady, this, &AppController::onApiResultsReady);
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
// onApiResultsReady
// ─────────────────────────────────────────────

void AppController::onApiResultsReady(const QJsonArray& results) {
    if (!m_db) return;

    // 1. Iterate over JSON and ensure products exist in our local SQLite
    //    so foreign keys (like daily_entries) still work.
    QVector<Product> existingProducts = m_db->getProducts();
    for (const QJsonValue& val : results) {
        QJsonObject obj = val.toObject();
        // Try "sku" (preferred, new format) then fall back to "product_id" (legacy)
        QString sku = obj.value(QStringLiteral("sku")).toString().trimmed();
        if (sku.isEmpty())
            sku = obj.value(QStringLiteral("product_id")).toString().trimmed();
        if (sku.isEmpty()) continue;
        
        // Find existing product id if any
        int existingId = 0;
        for (const auto& ep : existingProducts) {
            if (ep.sku == sku) {
                existingId = ep.id;
                break;
            }
        }
        
        Product p;
        p.id           = existingId;
        p.sku          = sku;
        p.name         = obj.value(QStringLiteral("product_name")).toString();
        if (p.name.isEmpty()) p.name = obj.value(QStringLiteral("name")).toString();
        p.category     = obj.value(QStringLiteral("category")).toString();
        p.supplier     = obj.value(QStringLiteral("supplier_name")).toString();
        p.currentStock = static_cast<int>(obj.value(QStringLiteral("current_stock")).toDouble());
        p.reorderPoint = static_cast<int>(obj.value(QStringLiteral("reorder_point")).toDouble());
        p.unitCost     = obj.value(QStringLiteral("unit_cost")).toDouble();
        
        m_db->saveProduct(p);
    }
    
    // 2. Save the full JSON prediction blob to local SQLite pipeline_results
    QJsonDocument doc(results);
    m_db->savePipelineResults(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
    
    // 3. Reload everything from local SQLite to ensure the entire app stays in sync
    loadFromDatabase();
}

// ─────────────────────────────────────────────
// loadFromDatabase
// ─────────────────────────────────────────────

bool AppController::loadFromDatabase() {
    if (!m_db) return false;
    QVector<Product> dbProds = m_db->getProducts();
    if (dbProds.isEmpty()) {
        // Database is empty. We will not seed dummy data,
        // so the app remains clean until the user imports a CSV.
    }

    // Try loading actual ML pipeline results from Database
    QString jsonStr = m_db->getLatestPipelineResults();
    bool loadedMLResults = false;
    if (!jsonStr.isEmpty()) {
        QJsonParseError err;
        QJsonDocument jdoc = QJsonDocument::fromJson(jsonStr.toUtf8(), &err);
        if (err.error == QJsonParseError::NoError && jdoc.isArray()) {
            QJsonArray arr = jdoc.array();
            for (const QJsonValue& val : arr) {
                QJsonObject obj = val.toObject();

                // ── SKU matching ─────────────────────────────────────────────
                // Stored JSON may use either "sku" (preferred, from fixed main.py)
                // or the old "product_id" key (legacy format). Try both.
                QString sku = obj.value(QStringLiteral("sku")).toString().trimmed();
                if (sku.isEmpty())
                    sku = obj.value(QStringLiteral("product_id")).toString().trimmed();

                for (auto& p : dbProds) {
                    if (p.sku.trimmed() != sku) continue;

                    // Keep the panel's inventory inputs aligned with the exact
                    // snapshot used to generate this model result.
                    if (obj.contains(QStringLiteral("current_stock")))
                        p.currentStock = static_cast<int>(obj.value(QStringLiteral("current_stock")).toDouble());
                    const int modelReorderPoint = static_cast<int>(obj.value(QStringLiteral("reorder_point")).toDouble());
                    if (modelReorderPoint > 0)
                        p.reorderPoint = modelReorderPoint;

                    // ── Demand Label ────────────────────────────────────────
                    QString dl = obj.value(QStringLiteral("demand_label")).toString().trimmed();
                    if      (dl.contains(QStringLiteral("High"),   Qt::CaseInsensitive)) p.demandLabel = DemandLabel::High;
                    else if (dl.contains(QStringLiteral("Low"),    Qt::CaseInsensitive)) p.demandLabel = DemandLabel::Low;
                    else if (dl.contains(QStringLiteral("Medium"), Qt::CaseInsensitive)) p.demandLabel = DemandLabel::Medium;
                    else                                                                   p.demandLabel = DemandLabel::Medium;

                    // ── Stock Status ─────────────────────────────────────────
                    // Python outputs: Critical / Low / Healthy / Overstock
                    // C++ enums:      Reorder / NoAction / Overstock
                    QString ss = obj.value(QStringLiteral("stock_status")).toString().trimmed();
                    if      (ss.contains(QStringLiteral("Critical"),  Qt::CaseInsensitive) ||
                             ss.contains(QStringLiteral("Reorder"),   Qt::CaseInsensitive) ||
                             ss.contains(QStringLiteral("Low"),       Qt::CaseInsensitive))
                        p.stockStatus = StockStatus::Reorder;
                    else if (ss.contains(QStringLiteral("Overstock"), Qt::CaseInsensitive))
                        p.stockStatus = StockStatus::Overstock;
                    else    // "Healthy", "No Action", "NoAction", unknown
                        p.stockStatus = StockStatus::NoAction;

                    // ── Priority ─────────────────────────────────────────────
                    // Stored key is now "priority" (fixed), legacy was "urgency"
                    QString pr = obj.value(QStringLiteral("priority")).toString().trimmed();
                    if (pr.isEmpty())
                        pr = obj.value(QStringLiteral("urgency")).toString().trimmed();
                    if      (pr.contains(QStringLiteral("Critical"),    Qt::CaseInsensitive))
                        p.priority = Priority::Critical;
                    else if (pr.contains(QStringLiteral("Reorder"),     Qt::CaseInsensitive) ||
                             pr.contains(QStringLiteral("Low"),         Qt::CaseInsensitive))
                        p.priority = Priority::ReorderSoon;
                    else    // "Safe", unknown
                        p.priority = Priority::Safe;

                    // ── Confidence ───────────────────────────────────────────
                    // Value stored as fraction (0.0–1.0); display needs 0–100.
                    double confRaw = obj.value(QStringLiteral("confidence")).toDouble();
                    p.confidence = (confRaw <= 1.0 && confRaw > 0.0) ? confRaw * 100.0 : confRaw;

                    // ── Forecast 7-day total ──────────────────────────────────
                    // New format: "forecast_7d", legacy: "weekly_forecast"
                    double fc = obj.value(QStringLiteral("forecast_7d")).toDouble();
                    if (fc == 0.0) fc = obj.value(QStringLiteral("weekly_forecast")).toDouble();
                    p.forecastNext7 = fc;

                    // ── Trend slope ───────────────────────────────────────────
                    // New format: "trend" (numeric), legacy: "trend_direction" (string)
                    QJsonValue trendVal = obj.value(QStringLiteral("trend"));
                    if (trendVal.isDouble()) {
                        p.forecastTrend = trendVal.toDouble();
                    } else {
                        // Map legacy string → ±0.5 as a neutral placeholder
                        QString td = obj.value(QStringLiteral("trend_direction")).toString();
                        p.forecastTrend = td.contains(QStringLiteral("Rising"), Qt::CaseInsensitive) ? 0.5 : -0.5;
                    }

                    // ── EOQ ───────────────────────────────────────────────────
                    // New format: "eoq" (int), legacy: "eoq_quantity" (float)
                    int eoqVal = obj.value(QStringLiteral("eoq")).toInt();
                    if (eoqVal == 0)
                        eoqVal = static_cast<int>(obj.value(QStringLiteral("eoq_quantity")).toDouble());
                    p.eoqQty = eoqVal;
                    p.usingDefaultEOQ = false;

                    // ── Text outputs ──────────────────────────────────────────
                    p.recommendation = obj.value(QStringLiteral("recommendation")).toString();

                    p.explanation = obj.value(QStringLiteral("explanation")).toString();
                    if (p.explanation.isEmpty())
                        p.explanation = obj.value(QStringLiteral("explanations")).toString();

                    // ── Forecast Points ───────────────────────────────────────
                    p.forecast.clear();
                    QJsonArray pts = obj.value(QStringLiteral("forecast_points")).toArray();
                    for (const QJsonValue& ptVal : pts) {
                        QJsonObject ptObj = ptVal.toObject();
                        ForecastPoint fp;
                        fp.date  = QDate::fromString(
                            ptObj.value(QStringLiteral("ds")).toString(),
                            QStringLiteral("yyyy-MM-dd"));
                        fp.value = ptObj.value(QStringLiteral("yhat")).toDouble();
                        fp.lower = ptObj.value(QStringLiteral("yhat_lower")).toDouble();
                        fp.upper = ptObj.value(QStringLiteral("yhat_upper")).toDouble();
                        // Clamp lower to 0 (Prophet can predict negative lower bound)
                        if (fp.lower < 0.0) fp.lower = 0.0;
                        if (fp.upper < fp.value) fp.upper = fp.value;
                        p.forecast.append(fp);
                    }

                    const double dailyBase = p.forecastNext7 / 7.0;
                    p.historicalSales = makeHistory(dailyBase, 30);
                    break;
                }
            }
            m_pipelineLive = true;
            loadedMLResults = true;
        }
    }


    if (!loadedMLResults) {
        // Assign safe, neutral default ML values for products if no pipeline results exist
        for (int i = 0; i < dbProds.size(); ++i) {
            Product& p = dbProds[i];
            // Default generated ML/forecasting values
            p.demandLabel = DemandLabel::Medium;
            p.stockStatus = StockStatus::NoAction;
            p.priority = Priority::Safe;
            p.confidence = 85.0;
            p.forecastNext7 = p.currentStock * 0.5;
            p.forecastTrend = 0.5;
            p.eoqQty = 50;

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
                p.demandLabel    = r.demandLabel;
                p.stockStatus    = r.stockStatus;
                p.confidence     = r.confidence;
                p.priority       = r.priority;
                p.forecastNext7  = r.forecastNext7;
                p.forecastTrend  = r.forecastTrend;
                p.forecast       = r.forecast;
                p.eoqQty         = r.eoqQty;
                p.usingDefaultEOQ= false;
                p.recommendation = r.recommendation;
                p.explanation    = r.explanation;

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
        p.reorderPoint   = 10;       // default reorder point for seed data
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

// ─────────────────────────────────────────────
// Search
// ─────────────────────────────────────────────

void AppController::setSearchQuery(const QString& query) {
    qDebug() << "[TRACE] AppController received search query:" << query;
    if (m_searchQuery != query) {
        m_searchQuery = query;
        emit searchQueryChanged(m_searchQuery);
        qDebug() << "[TRACE] AppController emitted searchQueryChanged:" << m_searchQuery;
    } else {
        qDebug() << "[TRACE] AppController query unchanged, not emitting.";
    }
}

bool AppController::matchesSearch(const Product& p, const QString& query) const {
    if (query.isEmpty()) return true;
    if (p.name.contains(query, Qt::CaseInsensitive))     return true;
    if (p.sku.contains(query, Qt::CaseInsensitive))      return true;
    if (p.category.contains(query, Qt::CaseInsensitive)) return true;
    if (p.supplier.contains(query, Qt::CaseInsensitive)) return true;
    if (QString::number(p.id).contains(query))           return true;

    // Also match stringified enumerations (e.g. "Safe", "Critical", "Overstock")
    if (toString(p.demandLabel).contains(query, Qt::CaseInsensitive)) return true;
    if (toString(p.stockStatus).contains(query, Qt::CaseInsensitive)) return true;
    if (toString(p.priority).contains(query, Qt::CaseInsensitive))    return true;

    return false;
}

} // namespace Kirana
