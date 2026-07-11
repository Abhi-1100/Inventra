#pragma once

#include <QString>
#include <QDate>
#include <QVector>
#include <QMetaType>

namespace Kirana {

// ─────────────────────────────────────────────
// Enumerations
// ─────────────────────────────────────────────

enum class DemandLabel : quint8 {
    High   = 0,
    Medium = 1,
    Low    = 2,
    Unknown = 3
};

enum class StockStatus : quint8 {
    Reorder   = 0,
    NoAction  = 1,
    Overstock = 2,
    Unknown   = 3
};

enum class Priority : quint8 {
    Critical    = 0,
    ReorderSoon = 1,
    Safe        = 2,
    Unknown     = 3
};

// ─────────────────────────────────────────────
// Prophet forecast point (one day)
// ─────────────────────────────────────────────

struct ForecastPoint {
    QDate  date;
    double value = 0.0;   // yhat
    double lower = 0.0;   // yhat_lower
    double upper = 0.0;   // yhat_upper
};

// ─────────────────────────────────────────────
// Core product record
// ─────────────────────────────────────────────

struct Product {
    int     id           = 0;
    QString sku;
    QString name;
    QString category;
    int     currentStock = 0;
    double  unitCost     = 0.0;

    // ML pipeline results
    DemandLabel demandLabel  = DemandLabel::Unknown;
    StockStatus stockStatus  = StockStatus::Unknown;
    double      confidence   = 0.0;    // 0–100 %
    Priority    priority     = Priority::Unknown;

    // Forecast
    double forecastNext7  = 0.0;   // 7-day total demand forecast
    double forecastTrend  = 0.0;   // units/day slope (+ve = rising)
    QVector<ForecastPoint> forecast;   // 7 daily points

    // Historical sales (last 30 days, units/day)
    QVector<double> historicalSales;

    // EOQ
    int    eoqQty          = 0;
    double orderingCost    = 20.0;   // ₹ per order (or $ depending on locale)
    double holdingCostRate = 0.25;   // % of unit cost per year
    bool   usingDefaultEOQ = true;   // flag: user hasn't customised costs yet
};

// ─────────────────────────────────────────────
// Helper string conversions
// ─────────────────────────────────────────────

inline QString toString(DemandLabel d) noexcept {
    switch (d) {
        case DemandLabel::High:   return QStringLiteral("High");
        case DemandLabel::Medium: return QStringLiteral("Medium");
        case DemandLabel::Low:    return QStringLiteral("Low");
        default:                  return QStringLiteral("—");
    }
}

inline QString toString(StockStatus s) noexcept {
    switch (s) {
        case StockStatus::Reorder:   return QStringLiteral("Reorder");
        case StockStatus::NoAction:  return QStringLiteral("No Action");
        case StockStatus::Overstock: return QStringLiteral("Overstock");
        default:                     return QStringLiteral("—");
    }
}

inline QString toString(Priority p) noexcept {
    switch (p) {
        case Priority::Critical:    return QStringLiteral("Critical");
        case Priority::ReorderSoon: return QStringLiteral("Reorder Soon");
        case Priority::Safe:        return QStringLiteral("Safe");
        default:                    return QStringLiteral("—");
    }
}

// ─────────────────────────────────────────────
// Terminal status palette (shared constants)
// ─────────────────────────────────────────────

namespace Palette {
    // Surfaces
    static constexpr const char* BgPrimary   = "#0d1117";
    static constexpr const char* BgSurface   = "#161b22";
    static constexpr const char* BgOverlay   = "#21262d";
    static constexpr const char* Border      = "#30363d";
    // Text
    static constexpr const char* TextPrimary   = "#e6edf3";
    static constexpr const char* TextSecondary = "#8b949e";
    static constexpr const char* TextMuted     = "#484f58";
    // Status
    static constexpr const char* Critical = "#f85149";
    static constexpr const char* Warning  = "#d29922";
    static constexpr const char* Success  = "#3fb950";
    static constexpr const char* Info     = "#58a6ff";
    static constexpr const char* Accent   = "#58a6ff";
} // namespace Palette

} // namespace Kirana

// Qt metatype registration (needed for queued signals across threads)
Q_DECLARE_METATYPE(Kirana::Product)
Q_DECLARE_METATYPE(QVector<Kirana::Product>)
