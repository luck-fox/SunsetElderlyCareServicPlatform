#include "httppushservice.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QUrl>
#include <QDebug>

HttpPushService::HttpPushService(QObject *parent) : QObject(parent)
{
    m_netManager = new QNetworkAccessManager(this);
    connect(m_netManager, &QNetworkAccessManager::finished, this, &HttpPushService::onPushFinished);
    
    m_processTimer = new QTimer(this);
    m_processTimer->setInterval(1000);
    connect(m_processTimer, &QTimer::timeout, this, &HttpPushService::processNextRequest);
    m_processTimer->start();
    
    qDebug() << "[HTTP推送] 模块初始化成功";
}

void HttpPushService::setServerUrl(const QString &baseUrl)
{
    m_baseUrl = baseUrl;
}

void HttpPushService::enqueueRequest(const QUrl &url, const QJsonObject &jsonData, int maxRetries)
{
    PushRequest req;
    req.url = url;
    req.jsonData = jsonData;
    req.maxRetries = maxRetries;
    
    m_requestQueue.enqueue(req);
    qDebug() << "[HTTP推送] 请求入队，当前队列大小：" << m_requestQueue.size();
}

void HttpPushService::processNextRequest()
{
    if (m_isProcessing || m_requestQueue.isEmpty()) {
        return;
    }
    
    m_isProcessing = true;
    PushRequest req = m_requestQueue.dequeue();
    
    QNetworkRequest request(req.url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QByteArray data = QJsonDocument(req.jsonData).toJson();
    
    QNetworkReply *reply = m_netManager->post(request, data);
    reply->setProperty("retryCount", req.retryCount);
    reply->setProperty("maxRetries", req.maxRetries);
    reply->setProperty("originalUrl", req.url.toString());
    reply->setProperty("requestData", data);
    
    qDebug() << "[HTTP推送] 发送请求：" << req.url.toString()
             << "重试次数：" << req.retryCount;
}

void HttpPushService::pushReport(const QString &familyPhone, const QString &elderName, const HealthData &data)
{
    QJsonObject jsonObj;
    jsonObj["type"] = "health_report";
    jsonObj["family_phone"] = familyPhone;
    jsonObj["elder_name"] = elderName;
    jsonObj["heart_rate"] = data.heartRate;
    jsonObj["blood_pressure"] = QString("%1/%2").arg(data.systolicPressure).arg(data.diastolicPressure);
    jsonObj["temperature"] = data.temperature;
    jsonObj["spo2"] = data.spO2;
    jsonObj["step_count"] = data.stepCount;
    jsonObj["emotion_level"] = static_cast<int>(data.emotionLevel);
    jsonObj["pressure_level"] = static_cast<int>(data.pressureLevel);
    jsonObj["battery"] = data.battery;
    jsonObj["record_time"] = data.recordTime.toString("yyyy-MM-dd HH:mm:ss");

    enqueueRequest(QUrl(m_baseUrl + "/health/report"), jsonObj);
}

void HttpPushService::pushAlert(const QString &familyPhone, const QString &elderName, const HealthData &data)
{
    QJsonObject jsonObj;
    jsonObj["type"] = "abnormal_alert";
    jsonObj["family_phone"] = familyPhone;
    jsonObj["elder_name"] = elderName;

    QStringList abnormalList;
    if (data.heartRate < 60 || data.heartRate > 100) abnormalList.append("心率异常");
    if (data.systolicPressure > 140) abnormalList.append("收缩压异常");
    if (data.diastolicPressure > 90) abnormalList.append("舒张压异常");
    if (data.temperature < 36.0 || data.temperature > 37.5) abnormalList.append("体温异常");
    if (data.spO2 < 95) abnormalList.append("血氧异常");
    if (data.battery < 20) abnormalList.append("手环电量低");
    if (data.pressureLevel >= PRESSURE_HIGH) abnormalList.append("高压状态");

    jsonObj["abnormal_info"] = abnormalList.join("、");
    jsonObj["heart_rate"] = data.heartRate;
    jsonObj["blood_pressure"] = QString("%1/%2").arg(data.systolicPressure).arg(data.diastolicPressure);
    jsonObj["temperature"] = data.temperature;
    jsonObj["spo2"] = data.spO2;
    jsonObj["alert_time"] = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

    enqueueRequest(QUrl(m_baseUrl + "/health/alert"), jsonObj, 5);
}

void HttpPushService::pushSOS(const QString &familyPhone, const QString &elderName, const QString &elderId)
{
    QJsonObject jsonObj;
    jsonObj["type"] = "sos_alert";
    jsonObj["family_phone"] = familyPhone;
    jsonObj["elder_name"] = elderName;
    jsonObj["elder_id"] = elderId;
    jsonObj["sos_time"] = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    jsonObj["message"] = "紧急求助！请立即联系老人或前往查看。";

    enqueueRequest(QUrl(m_baseUrl + "/health/sos"), jsonObj, 10);
}

void HttpPushService::onPushFinished(QNetworkReply *reply)
{
    m_isProcessing = false;
    
    int retryCount = reply->property("retryCount").toInt();
    int maxRetries = reply->property("maxRetries").toInt();
    QString urlStr = reply->property("originalUrl").toString();
    QByteArray requestData = reply->property("requestData").toByteArray();

    if (reply->error() == QNetworkReply::NoError) {
        qDebug() << "[HTTP推送] 成功：" << reply->readAll();
    } else {
        qWarning() << "[HTTP推送] 失败：" << reply->errorString() << "URL：" << urlStr;
        
        if (retryCount < maxRetries) {
            PushRequest req;
            req.url = QUrl(urlStr);
            req.jsonData = QJsonDocument::fromJson(requestData).object();
            req.retryCount = retryCount + 1;
            req.maxRetries = maxRetries;
            
            m_requestQueue.prepend(req);
            qDebug() << "[HTTP推送] 请求重新入队，重试次数：" << req.retryCount;
        } else {
            qCritical() << "[HTTP推送] 请求最终失败，已达最大重试次数：" << maxRetries;
        }
    }
    reply->deleteLater();
}
