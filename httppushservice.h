#ifndef HTTPPUSHSERVICE_H
#define HTTPPUSHSERVICE_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QQueue>
#include <QTimer>
#include <QUrl>
#include <QJsonObject>
#include <QHash>
#include <QDateTime>
#include "healthdatamanager.h"

struct PushRequest {
    QUrl url;
    QJsonObject jsonData;
    int retryCount = 0;
    int maxRetries = 3;
};

class HttpPushService : public QObject
{
    Q_OBJECT
public:
    explicit HttpPushService(QObject *parent = nullptr);

    void setServerUrl(const QString &baseUrl);
    
    void pushReport(const QString &familyPhone, const QString &elderName, const HealthData &data);
    void pushAlert(const QString &familyPhone, const QString &elderName, const HealthData &data);
    void pushSOS(const QString &familyPhone, const QString &elderName, const QString &elderId);
    
    int getPendingCount() const { return m_requestQueue.size(); }

private slots:
    void onPushFinished(QNetworkReply *reply);
    void processNextRequest();

private:
    void enqueueRequest(const QUrl &url, const QJsonObject &jsonData, int maxRetries = 3);
    
    QNetworkAccessManager *m_netManager;
    QQueue<PushRequest> m_requestQueue;
    QTimer *m_processTimer;
    bool m_isProcessing = false;
    QString m_baseUrl = "http://127.0.0.1:8080";

    // 告警去重:同一老人同一类异常在 ALERT_DEDUP_SECONDS 秒内不重复推送
    QHash<QString, QDateTime> m_lastAlertTime;
    static const int ALERT_DEDUP_SECONDS = 300; // 5 分钟
};

#endif
