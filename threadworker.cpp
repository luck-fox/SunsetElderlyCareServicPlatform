#include "threadworker.h"
#include <QStringList>
#include <QDebug>

ThreadWorker::ThreadWorker(QObject *parent) : QObject(parent) {}

void ThreadWorker::setHealthDataManager(HealthDataManager *manager)
{
    m_healthMgr = manager;
    m_useExternalManager = true;
}

void ThreadWorker::doParseAndSave(const QByteArray &data)
{
    QString strData = QString::fromUtf8(data).trimmed();
    QStringList dataList = strData.split(",");

    if (dataList.size() != 11) {
        QString error = QString("数据格式错误：%1（需11个字段，实际：%2）")
                           .arg(strData).arg(dataList.size());
        qWarning() << error;
        emit parseError(error);
        emit dataSaved(false);
        return;
    }

    HealthData healthData;
    healthData.elderId = dataList[0];
    
    bool ok = true;
    healthData.heartRate = dataList[1].toInt(&ok);
    if (!ok) {
        emit parseError("心率数据格式错误");
        emit dataSaved(false);
        return;
    }
    
    healthData.systolicPressure = dataList[2].toInt(&ok);
    healthData.diastolicPressure = dataList[3].toInt(&ok);
    healthData.temperature = dataList[4].toDouble(&ok);
    healthData.spO2 = dataList[5].toInt(&ok);
    healthData.stepCount = dataList[6].toInt(&ok);
    healthData.emotionLevel = static_cast<EmotionLevel>(dataList[7].toInt(&ok));
    healthData.pressureLevel = static_cast<PressureLevel>(dataList[8].toInt(&ok));
    healthData.battery = dataList[9].toInt(&ok);
    healthData.sosStatus = (dataList[10].toInt(&ok) == 1);
    healthData.recordTime = QDateTime::currentDateTime();

    QStringList errors;
    if (!validateData(healthData, errors)) {
        for (const QString &err : errors) {
            qWarning() << "数据校验警告：" << err;
        }
    }

    bool saveOk = false;
    if (m_useExternalManager && m_healthMgr) {
        saveOk = m_healthMgr->insertHealthData(healthData);
    } else {
        HealthDataManager manager;
        saveOk = manager.insertHealthData(healthData);
    }

    emit dataParsed(healthData.elderId, healthData);

    if (healthData.sosStatus) {
        emit sosParsed(healthData.elderId);
    }

    emit dataSaved(saveOk);

    qDebug() << "[子线程] 数据解析存储完成：" << healthData.elderId
             << "心率：" << healthData.heartRate
             << "体温：" << healthData.temperature;
}

bool ThreadWorker::validateData(const HealthData &data, QStringList &errors)
{
    bool valid = true;

    if (data.elderId.isEmpty()) {
        errors.append("老人ID为空");
        valid = false;
    }

    if (data.heartRate < 30 || data.heartRate > 200) {
        errors.append(QString("心率超出合理范围：%1").arg(data.heartRate));
    }

    if (data.systolicPressure < 50 || data.systolicPressure > 250) {
        errors.append(QString("收缩压超出合理范围：%1").arg(data.systolicPressure));
    }

    if (data.diastolicPressure < 30 || data.diastolicPressure > 150) {
        errors.append(QString("舒张压超出合理范围：%1").arg(data.diastolicPressure));
    }

    if (data.temperature < 30.0 || data.temperature > 45.0) {
        errors.append(QString("体温超出合理范围：%1").arg(data.temperature));
    }

    if (data.spO2 < 50 || data.spO2 > 100) {
        errors.append(QString("血氧超出合理范围：%1").arg(data.spO2));
    }

    if (data.battery < 0 || data.battery > 100) {
        errors.append(QString("电量超出合理范围：%1").arg(data.battery));
    }

    if (data.stepCount < 0) {
        errors.append(QString("步数为负数：%1").arg(data.stepCount));
    }

    return valid;
}
