#pragma once

#include <QObject>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>

class QNetworkAccessManager;
class QNetworkReply;

namespace Kirana {

// ─────────────────────────────────────────────
// ApiClient — Singleton HTTP client for FastAPI
//
// Provides async methods that communicate with
// the FastAPI ML server at http://localhost:8000.
// All responses come back via Qt signals.
// ─────────────────────────────────────────────

class ApiClient : public QObject {
    Q_OBJECT

public:
    static ApiClient& instance();

    // ── API methods (all async) ─────────────────

    // Check if FastAPI is running
    void checkHealth();

    // Upload CSV and run pipeline
    void uploadCsv(const QString& csvPath);

    // Get all predictions
    void fetchAllResults();

    // Get single product
    void fetchProductResult(const QString& productId);

    // Get analytics summary
    void fetchAnalytics();

signals:
    // Emitted when health check done
    void healthChecked(bool isOnline);

    // Emitted when CSV upload + pipeline done
    void pipelineComplete(QJsonArray results);

    // Emitted when results fetched
    void resultsReady(QJsonArray results);

    // Emitted when single product fetched
    void productResultReady(QJsonObject result);

    // Emitted when analytics fetched
    void analyticsReady(QJsonObject analytics);

    // Emitted on any error
    void apiError(QString errorMessage);

private:
    explicit ApiClient(QObject* parent = nullptr);
    ~ApiClient() override = default;

    // Prevent copy
    ApiClient(const ApiClient&) = delete;
    ApiClient& operator=(const ApiClient&) = delete;

    void handleReply(QNetworkReply* reply,
                     const QString& operationName,
                     std::function<void(const QByteArray&)> onSuccess);

    QNetworkAccessManager* m_manager = nullptr;
    const QString BASE_URL = QStringLiteral("http://localhost:8000");

    static constexpr int REQUEST_TIMEOUT_MS = 30000; // 30 seconds
};

} // namespace Kirana
