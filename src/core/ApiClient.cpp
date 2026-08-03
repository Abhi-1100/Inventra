#include "core/ApiClient.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QHttpMultiPart>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QTimer>
#include <QDebug>

namespace Kirana {

// ─────────────────────────────────────────────
// Singleton
// ─────────────────────────────────────────────

ApiClient& ApiClient::instance() {
    static ApiClient s_instance;
    return s_instance;
}

ApiClient::ApiClient(QObject* parent)
    : QObject(parent)
    , m_manager(new QNetworkAccessManager(this))
{
    qDebug() << "[ApiClient] Initialized. Base URL:" << BASE_URL;
}

// ─────────────────────────────────────────────
// handleReply — Common response handler
// ─────────────────────────────────────────────

void ApiClient::handleReply(QNetworkReply* reply,
                            const QString& operationName,
                            std::function<void(const QByteArray&)> onSuccess)
{
    connect(reply, &QNetworkReply::finished, this, [this, reply, operationName, onSuccess]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            QString errMsg = QString("[ApiClient] %1 failed: %2")
                                 .arg(operationName, reply->errorString());
            qWarning() << errMsg;
            emit apiError(errMsg);
            return;
        }

        QByteArray data = reply->readAll();
        qDebug() << "[ApiClient]" << operationName << "response:" << data.left(200) << "...";

        onSuccess(data);
    });
}

// ─────────────────────────────────────────────
// checkHealth — GET /
// ─────────────────────────────────────────────

void ApiClient::checkHealth() {
    QNetworkRequest req(QUrl(BASE_URL + "/"));
    req.setTransferTimeout(5000); // 5s for health check

    QNetworkReply* reply = m_manager->get(req);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "[ApiClient] Health check failed:" << reply->errorString();
            emit healthChecked(false);
            return;
        }

        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        bool isOk = doc.object().value("status").toString() == "ok";
        qDebug() << "[ApiClient] Health check:" << (isOk ? "ONLINE" : "OFFLINE");
        emit healthChecked(isOk);
    });
}

// ─────────────────────────────────────────────
// uploadCsv — POST /upload_csv (multipart)
// ─────────────────────────────────────────────

void ApiClient::uploadCsv(const QString& csvPath) {
    QFile* file = new QFile(csvPath);
    if (!file->open(QIODevice::ReadOnly)) {
        emit apiError(QString("Cannot open file: %1").arg(csvPath));
        delete file;
        return;
    }

    qDebug() << "[ApiClient] Uploading CSV:" << csvPath;

    QHttpMultiPart* multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                       QVariant(QString("form-data; name=\"file\"; filename=\"%1\"")
                                    .arg(QFileInfo(csvPath).fileName())));
    filePart.setHeader(QNetworkRequest::ContentTypeHeader,
                       QVariant("text/csv"));
    filePart.setBodyDevice(file);
    file->setParent(multiPart); // multiPart takes ownership

    multiPart->append(filePart);

    QNetworkRequest req(QUrl(BASE_URL + "/upload_csv"));
    req.setTransferTimeout(REQUEST_TIMEOUT_MS);

    QNetworkReply* reply = m_manager->post(req, multiPart);
    multiPart->setParent(reply); // reply takes ownership of multiPart

    handleReply(reply, "uploadCsv", [this](const QByteArray& data) {
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject obj = doc.object();

        if (obj.value("status").toString() == "success") {
            QJsonArray predictions = obj.value("predictions").toArray();
            qDebug() << "[ApiClient] Pipeline complete." << predictions.size() << "predictions.";
            emit pipelineComplete(predictions);
            
            // Automatically refresh global data so the dashboard and analytics widgets update
            fetchAllResults();
            fetchAnalytics();
        } else {
            emit apiError("Pipeline returned non-success status.");
        }
    });
}

// ─────────────────────────────────────────────
// fetchAllResults — GET /results
// ─────────────────────────────────────────────

void ApiClient::fetchAllResults() {
    QNetworkRequest req(QUrl(BASE_URL + "/results"));
    req.setTransferTimeout(REQUEST_TIMEOUT_MS);

    QNetworkReply* reply = m_manager->get(req);

    handleReply(reply, "fetchAllResults", [this](const QByteArray& data) {
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isArray()) {
            QJsonArray arr = doc.array();
            qDebug() << "[ApiClient] Fetched" << arr.size() << "results.";
            emit resultsReady(arr);
        } else {
            emit apiError("GET /results did not return a JSON array.");
        }
    });
}

// ─────────────────────────────────────────────
// fetchProductResult — GET /results/{product_id}
// ─────────────────────────────────────────────

void ApiClient::fetchProductResult(const QString& productId) {
    QNetworkRequest req(QUrl(BASE_URL + "/results/" + productId));
    req.setTransferTimeout(REQUEST_TIMEOUT_MS);

    QNetworkReply* reply = m_manager->get(req);

    handleReply(reply, "fetchProductResult", [this](const QByteArray& data) {
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            qDebug() << "[ApiClient] Fetched single product result.";
            emit productResultReady(doc.object());
        } else {
            emit apiError("GET /results/{id} did not return a JSON object.");
        }
    });
}

// ─────────────────────────────────────────────
// fetchAnalytics — GET /analytics
// ─────────────────────────────────────────────

void ApiClient::fetchAnalytics() {
    QNetworkRequest req(QUrl(BASE_URL + "/analytics"));
    req.setTransferTimeout(REQUEST_TIMEOUT_MS);

    QNetworkReply* reply = m_manager->get(req);

    handleReply(reply, "fetchAnalytics", [this](const QByteArray& data) {
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            qDebug() << "[ApiClient] Fetched analytics.";
            emit analyticsReady(doc.object());
        } else {
            emit apiError("GET /analytics did not return a JSON object.");
        }
    });
}

} // namespace Kirana
