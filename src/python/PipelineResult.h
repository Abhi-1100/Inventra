#pragma once

#include <QString>
#include <QVector>
#include <QDate>
#include "core/ProductData.h"

namespace Kirana {

// ─────────────────────────────────────────────
// Per-product result from the ML pipeline
// ─────────────────────────────────────────────

struct MLResult {
    int         productId;
    DemandLabel demandLabel;
    StockStatus stockStatus;
    double      confidence;      // 0–100
    Priority    priority;
    int         eoqQty;

    // 7-day Prophet forecast
    QVector<ForecastPoint> forecast;
    double forecastNext7;
    double forecastTrend;
};

// ─────────────────────────────────────────────
// Full pipeline run result
// ─────────────────────────────────────────────

struct PipelineRunResult {
    bool             success    = false;
    QString          errorMsg;
    int              totalRows  = 0;
    QVector<MLResult> results;
    qint64           durationMs = 0;   // wall-clock time of the run
};

} // namespace Kirana

Q_DECLARE_METATYPE(Kirana::PipelineRunResult)
