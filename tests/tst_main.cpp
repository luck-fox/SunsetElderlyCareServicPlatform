#include <QtTest>
#include <QCoreApplication>
#include <QDateTime>
#include <QVector>
#include "healthdatamanager.h"
#include "httppushservice.h"

// 工具函数：构造一组正常生理指标
static HealthData makeNormalData(const QString &elderId, int hr = 75)
{
    HealthData data;
    data.elderId = elderId;
    data.recordTime = QDateTime::currentDateTime();
    data.heartRate = hr;
    data.systolicPressure = 120;
    data.diastolicPressure = 80;
    data.temperature = 36.5;
    data.spO2 = 98;
    data.stepCount = 5000;
    data.emotionLevel = EMOTION_NORMAL;
    data.pressureLevel = PRESSURE_NONE;
    data.battery = 85;
    data.sosStatus = false;
    return data;
}

// ==================== HealthDataManager 测试 ====================
class TestHealthDataManager : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase()
    {
        m_mgr = new HealthDataManager();
        QVERIFY2(m_mgr->initDatabase(), "数据库初始化失败");
    }

    void cleanupTestCase()
    {
        if (m_mgr) {
            m_mgr->deleteHealthDataByElderId("TST001");
            m_mgr->deleteHealthDataByElderId("TST002");
            m_mgr->deleteHealthDataByElderId("TST003");
            delete m_mgr;
            m_mgr = nullptr;
        }
    }

    void testInsertAndQuery()
    {
        m_mgr->deleteHealthDataByElderId("TST001");

        HealthData data = makeNormalData("TST001", 75);
        QVERIFY(m_mgr->insertHealthData(data));

        QDateTime start = data.recordTime.addSecs(-1);
        QDateTime end   = data.recordTime.addSecs(1);
        QVector<HealthData> results = m_mgr->queryHealthData("TST001", start, end);

        QCOMPARE(results.size(), 1);
        QCOMPARE(results[0].elderId, QString("TST001"));
        QCOMPARE(results[0].heartRate, 75);
        QCOMPARE(results[0].systolicPressure, 120);
        QCOMPARE(results[0].temperature, 36.5);
    }

    void testGetLatestData()
    {
        m_mgr->deleteHealthDataByElderId("TST002");

        // 插入 3 条记录，时间递增、心率递增
        for (int i = 0; i < 3; ++i) {
            HealthData data = makeNormalData("TST002", 70 + i);
            data.recordTime = QDateTime::currentDateTime().addSecs(i * 10);
            QVERIFY(m_mgr->insertHealthData(data));
        }

        HealthData latest = m_mgr->getLatestData("TST002");
        QVERIFY(latest.id > 0);
        QCOMPARE(latest.elderId, QString("TST002"));
        QCOMPARE(latest.heartRate, 72); // 最后插入的记录心率最大
    }

    void testCheckAbnormalData()
    {
        // 正常数据
        HealthData normal = makeNormalData("TST_CHK", 75);
        QVERIFY(!m_mgr->checkAbnormalData(normal));

        // 心率过高
        HealthData highHr = makeNormalData("TST_CHK", 150);
        QVERIFY(m_mgr->checkAbnormalData(highHr));

        // 心率过低
        HealthData lowHr = makeNormalData("TST_CHK", 40);
        QVERIFY(m_mgr->checkAbnormalData(lowHr));

        // 血氧过低
        HealthData lowSpo2 = makeNormalData("TST_CHK", 75);
        lowSpo2.spO2 = 90;
        QVERIFY(m_mgr->checkAbnormalData(lowSpo2));

        // 体温异常
        HealthData highTemp = makeNormalData("TST_CHK", 75);
        highTemp.temperature = 38.5;
        QVERIFY(m_mgr->checkAbnormalData(highTemp));

        // 电量低
        HealthData lowBatt = makeNormalData("TST_CHK", 75);
        lowBatt.battery = 10;
        QVERIFY(m_mgr->checkAbnormalData(lowBatt));

        // SOS 触发
        HealthData sos = makeNormalData("TST_CHK", 75);
        sos.sosStatus = true;
        QVERIFY(m_mgr->checkAbnormalData(sos));

        // 压力过高
        HealthData highPressure = makeNormalData("TST_CHK", 75);
        highPressure.pressureLevel = PRESSURE_HIGH;
        QVERIFY(m_mgr->checkAbnormalData(highPressure));
    }

    void testBatchInsert()
    {
        m_mgr->deleteHealthDataByElderId("TST003");

        QVector<HealthData> batch;
        for (int i = 0; i < 5; ++i) {
            HealthData data = makeNormalData("TST003", 70 + i);
            data.recordTime = QDateTime::currentDateTime().addSecs(i * 60);
            batch.append(data);
        }
        QVERIFY(m_mgr->insertHealthDataBatch(batch));

        QDateTime start = QDateTime::currentDateTime().addSecs(-1);
        QDateTime end   = QDateTime::currentDateTime().addSecs(5 * 60 + 1);
        QVector<HealthData> results = m_mgr->queryHealthData("TST003", start, end);
        QCOMPARE(results.size(), 5);
    }

    void testStatistics()
    {
        m_mgr->deleteHealthDataByElderId("TST003");

        // 3 条正常 + 1 条异常
        for (int i = 0; i < 3; ++i) {
            HealthData data = makeNormalData("TST003", 70 + i);
            data.recordTime = QDateTime::currentDateTime().addSecs(i * 10);
            QVERIFY(m_mgr->insertHealthData(data));
        }
        HealthData abn = makeNormalData("TST003", 150);
        abn.recordTime = QDateTime::currentDateTime().addSecs(40);
        QVERIFY(m_mgr->insertHealthData(abn));

        QDateTime start = QDateTime::currentDateTime().addSecs(-1);
        QDateTime end   = QDateTime::currentDateTime().addSecs(60);
        HealthDataManager::HealthStatistics stats = m_mgr->getStatistics("TST003", start, end);

        QCOMPARE(stats.totalRecords, 4);
        QCOMPARE(stats.abnormalCount, 1);
        QVERIFY(stats.maxHeartRate == 150.0);
        QVERIFY(stats.minHeartRate == 70.0);
    }

private:
    HealthDataManager *m_mgr = nullptr;
};

// ==================== HttpPushService 测试 ====================
class TestHttpPushService : public QObject
{
    Q_OBJECT
private slots:
    void testPushReportEnqueue()
    {
        HttpPushService service;
        service.setServerUrl("http://127.0.0.1:8080");
        QCOMPARE(service.getPendingCount(), 0);

        HealthData data = makeNormalData("TST001", 75);
        service.pushReport("13800138000", "测试老人", data);
        QCOMPARE(service.getPendingCount(), 1);
    }

    void testPushSOS()
    {
        HttpPushService service;
        service.setServerUrl("http://127.0.0.1:8080");
        QCOMPARE(service.getPendingCount(), 0);

        service.pushSOS("13800138000", "测试老人", "TST001");
        QCOMPARE(service.getPendingCount(), 1);
    }

    void testPushAlertDedup()
    {
        HttpPushService service;
        service.setServerUrl("http://127.0.0.1:8080");

        HealthData abnormal = makeNormalData("TST001", 150); // 心率过高 → 异常

        // 第一次告警：入队
        service.pushAlert("13800138000", "测试老人", abnormal);
        QCOMPARE(service.getPendingCount(), 1);

        // 同类告警 5 分钟内：去重，不入队
        service.pushAlert("13800138000", "测试老人", abnormal);
        QCOMPARE(service.getPendingCount(), 1); // 仍为 1，未增加
    }

    void testDifferentAlertNotDeduped()
    {
        HttpPushService service;
        service.setServerUrl("http://127.0.0.1:8080");

        // 告警 1：心率异常
        HealthData alert1 = makeNormalData("TST001", 150);
        service.pushAlert("13800138000", "测试老人", alert1);
        QCOMPARE(service.getPendingCount(), 1);

        // 告警 2：血氧异常（异常类型不同，key 不同，不去重）
        HealthData alert2 = makeNormalData("TST001", 75);
        alert2.spO2 = 90;
        service.pushAlert("13800138000", "测试老人", alert2);
        QCOMPARE(service.getPendingCount(), 2);
    }
};

// ==================== 测试入口 ====================
int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    int result = 0;

    {
        TestHealthDataManager t;
        result |= QTest::qExec(&t, argc, argv);
    }
    {
        TestHttpPushService t;
        result |= QTest::qExec(&t, argc, argv);
    }
    return result;
}

#include "tst_main.moc"
