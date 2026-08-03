#pragma once

#include <QObject>
#include <QDateTime>
#include "core/ProductData.h"
#include "python/PipelineResult.h"

namespace Kirana {

class Database;

// ─────────────────────────────────────────────
// AppSettings (shared mutable config)
// ─────────────────────────────────────────────

struct AppSettings {
    double orderingCost     = 20.0;    // ₹/order
    double holdingCostRate  = 0.25;    // fraction of unit cost/year
    int    leadTimeDays     = 7;
    double reorderThreshHigh   = 0.80; // confidence threshold → Critical
    double reorderThreshMedium = 0.60;
    int    forecastHorizonDays = 7;
    int    sessionTimeoutMinutes = 5; // 0 means Never
    QString dbPath;                    // SQLite path
};

// ─────────────────────────────────────────────
// AppController
// Central state manager for the application.
// Connects UI to Database and manages products list.
// ─────────────────────────────────────────────

class AppController : public QObject {
    Q_OBJECT

public:
    explicit AppController(Database* db, QObject* parent = nullptr);

    // ── Accessors ──────────────────────────
    const QVector<Product>& products() const { return m_products; }
    AppSettings&        settings()     { return m_settings; }
    const AppSettings&  settings() const { return m_settings; }
    Database*           database()     { return m_db; }
    QString             searchQuery() const { return m_searchQuery; }
    bool                matchesSearch(const Product& p, const QString& query) const;

    QDateTime lastRunTime() const { return m_lastRunTime; }
    bool      pipelineLive() const { return m_pipelineLive; }

    // ── Aggregate stats (computed on demand) ──
    int criticalCount()  const;
    int overstockCount() const;
    double stockoutRiskPct() const;

    // ── Actions ──────────────────────────
    void loadDummyData();
    bool loadFromDatabase();
    void applyPipelineResults(QVector<Product> updated);
    void applyPipelineRun(const PipelineRunResult& result);
    void updateSettings(const AppSettings& s);

public slots:
    void onApiResultsReady(const QJsonArray& results);
    void setSearchQuery(const QString& query);

signals:
    void productsChanged(const QVector<Product>& products);
    void pipelineStateChanged(bool live, QDateTime lastRun);
    void settingsChanged(const AppSettings& s);
    void searchQueryChanged(const QString& query);

private:
    static QVector<Product> buildDummyProducts(const AppSettings& cfg);
    static QVector<ForecastPoint> makeForecast(double base, double trend, int days);
    static QVector<double> makeHistory(double base, int days);

    Database*        m_db = nullptr;
    QVector<Product> m_products;
    AppSettings      m_settings;
    QDateTime        m_lastRunTime;
    bool             m_pipelineLive = false;
    QString          m_searchQuery;
};

} // namespace Kirana
