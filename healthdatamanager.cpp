#include "healthdatamanager.h"
#include <QSqlError>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QSqlDriver>
#include <QCoreApplication>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QStringEncoder>
#endif

HealthDataManager::HealthDataManager(QObject *parent) : QObject(parent)
{
}

HealthDataManager::~HealthDataManager()
{
    QString connectionName = m_db.connectionName();
    if (m_db.isOpen()) {
        m_db.close();
    }
    QSqlDatabase::removeDatabase(connectionName);
}

bool HealthDataManager::initDatabase()
{
    QString connectionName = "SunsetElderlyCareConnection";
    
    if (QSqlDatabase::contains(connectionName)) {
        m_db = QSqlDatabase::database(connectionName);
        if (m_db.isOpen()) {
            return true;
        }
    } else {
        m_db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    }
    
    if (!m_db.isValid()) {
        qCritical() << "SQLite驱动无效";
        return false;
    }
    
    if (!m_db.isOpen()) {
        QString dbPath = QCoreApplication::applicationDirPath() + "/智慧夕阳健康数据.db";
        m_db.setDatabaseName(dbPath);

        if (!m_db.open()) {
            qCritical() << "数据库打开失败：" << m_db.lastError().text();
            return false;
        }
        
        qDebug() << "数据库打开成功：" << dbPath;
    }

    QSqlQuery query(m_db);
    QString createElderTable = R"(
        CREATE TABLE IF NOT EXISTS elder_info (
            elder_id TEXT PRIMARY KEY,
            name TEXT NOT NULL,
            age INTEGER DEFAULT 0,
            gender TEXT DEFAULT '',
            phone TEXT DEFAULT '',
            emergency_contact TEXT DEFAULT '',
            emergency_phone TEXT DEFAULT '',
            address TEXT DEFAULT '',
            notes TEXT DEFAULT ''
        )
    )";

    if (!query.exec(createElderTable)) {
        qCritical() << "创建老人信息表失败：" << query.lastError().text();
        return false;
    }

    QString createSql = R"(
        CREATE TABLE IF NOT EXISTS health_data (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            elder_id TEXT NOT NULL,
            record_time DATETIME NOT NULL,
            heart_rate INTEGER NOT NULL,
            systolic_pressure INTEGER NOT NULL,
            diastolic_pressure INTEGER NOT NULL,
            temperature REAL NOT NULL,
            spo2 INTEGER NOT NULL,
            step_count INTEGER NOT NULL,
            emotion_level INTEGER NOT NULL,
            pressure_level INTEGER NOT NULL,
            battery INTEGER NOT NULL,
            sos_status INTEGER NOT NULL
        )
    )";

    if (!query.exec(createSql)) {
        qCritical() << "创建数据表失败：" << query.lastError().text();
        return false;
    }
    
    createIndexes();
    return true;
}

bool HealthDataManager::createIndexes()
{
    QSqlQuery query(m_db);
    QStringList indexSqls = {
        "CREATE INDEX IF NOT EXISTS idx_elder_id ON health_data(elder_id)",
        "CREATE INDEX IF NOT EXISTS idx_record_time ON health_data(record_time)",
        "CREATE INDEX IF NOT EXISTS idx_elder_time ON health_data(elder_id, record_time)"
    };
    
    for (const QString &sql : indexSqls) {
        if (!query.exec(sql)) {
            qWarning() << "创建索引失败：" << query.lastError().text();
            return false;
        }
    }
    return true;
}

bool HealthDataManager::insertHealthData(const HealthData &data)
{
    if (!m_db.isOpen()) {
        qWarning() << "数据库未打开";
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO health_data (
            elder_id, record_time, heart_rate, systolic_pressure, diastolic_pressure,
            temperature, spo2, step_count, emotion_level, pressure_level, battery, sos_status
        ) VALUES (
            :elderId, :recordTime, :heartRate, :systolic, :diastolic,
            :temperature, :spo2, :stepCount, :emotion, :pressure, :battery, :sos
        )
    )");

    // 绑定参数
    query.bindValue(":elderId", data.elderId);
    query.bindValue(":recordTime", data.recordTime);
    query.bindValue(":heartRate", data.heartRate);
    query.bindValue(":systolic", data.systolicPressure);
    query.bindValue(":diastolic", data.diastolicPressure);
    query.bindValue(":temperature", data.temperature);
    query.bindValue(":spo2", data.spO2);
    query.bindValue(":stepCount", data.stepCount);
    query.bindValue(":emotion", static_cast<int>(data.emotionLevel));
    query.bindValue(":pressure", static_cast<int>(data.pressureLevel));
    query.bindValue(":battery", data.battery);
    query.bindValue(":sos", data.sosStatus ? 1 : 0);

    if (!query.exec()) {
        qCritical() << "插入数据失败：" << query.lastError().text();
        return false;
    }
    return true;
}

bool HealthDataManager::insertHealthDataBatch(const QVector<HealthData> &dataList)
{
    if (!m_db.isOpen()) {
        qWarning() << "数据库未打开";
        return false;
    }

    m_db.transaction();
    QSqlQuery query(m_db);
    bool success = true;

    for (const HealthData &data : dataList) {
        query.prepare(R"(
            INSERT INTO health_data (
                elder_id, record_time, heart_rate, systolic_pressure, diastolic_pressure,
                temperature, spo2, step_count, emotion_level, pressure_level, battery, sos_status
            ) VALUES (
                :elderId, :recordTime, :heartRate, :systolic, :diastolic,
                :temperature, :spo2, :stepCount, :emotion, :pressure, :battery, :sos
            )
        )");

        query.bindValue(":elderId", data.elderId);
        query.bindValue(":recordTime", data.recordTime);
        query.bindValue(":heartRate", data.heartRate);
        query.bindValue(":systolic", data.systolicPressure);
        query.bindValue(":diastolic", data.diastolicPressure);
        query.bindValue(":temperature", data.temperature);
        query.bindValue(":spo2", data.spO2);
        query.bindValue(":stepCount", data.stepCount);
        query.bindValue(":emotion", static_cast<int>(data.emotionLevel));
        query.bindValue(":pressure", static_cast<int>(data.pressureLevel));
        query.bindValue(":battery", data.battery);
        query.bindValue(":sos", data.sosStatus ? 1 : 0);

        if (!query.exec()) {
            success = false;
            qCritical() << "批量插入数据失败：" << query.lastError().text();
            break;
        }
    }

    if (success) {
        m_db.commit();
        qDebug() << "批量插入数据成功，共" << dataList.size() << "条";
    } else {
        m_db.rollback();
        qWarning() << "批量插入数据失败，已回滚";
    }
    return success;
}

bool HealthDataManager::deleteHealthDataById(int id)
{
    if (!m_db.isOpen()) {
        qWarning() << "数据库未打开";
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare("DELETE FROM health_data WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        qCritical() << "删除数据失败：" << query.lastError().text();
        return false;
    }
    
    qDebug() << "删除数据成功，ID：" << id;
    return true;
}

bool HealthDataManager::deleteHealthDataByElderId(const QString &elderId)
{
    if (!m_db.isOpen()) {
        qWarning() << "数据库未打开";
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare("DELETE FROM health_data WHERE elder_id = :elderId");
    query.bindValue(":elderId", elderId);

    if (!query.exec()) {
        qCritical() << "删除老人数据失败：" << query.lastError().text();
        return false;
    }
    
    qDebug() << "删除老人数据成功，老人ID：" << elderId << "，删除" << query.numRowsAffected() << "条";
    return true;
}

bool HealthDataManager::updateHealthData(const HealthData &data)
{
    if (!m_db.isOpen()) {
        qWarning() << "数据库未打开";
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare(R"(
        UPDATE health_data SET
            elder_id = :elderId,
            record_time = :recordTime,
            heart_rate = :heartRate,
            systolic_pressure = :systolic,
            diastolic_pressure = :diastolic,
            temperature = :temperature,
            spo2 = :spo2,
            step_count = :stepCount,
            emotion_level = :emotion,
            pressure_level = :pressure,
            battery = :battery,
            sos_status = :sos
        WHERE id = :id
    )");

    query.bindValue(":id", data.id);
    query.bindValue(":elderId", data.elderId);
    query.bindValue(":recordTime", data.recordTime);
    query.bindValue(":heartRate", data.heartRate);
    query.bindValue(":systolic", data.systolicPressure);
    query.bindValue(":diastolic", data.diastolicPressure);
    query.bindValue(":temperature", data.temperature);
    query.bindValue(":spo2", data.spO2);
    query.bindValue(":stepCount", data.stepCount);
    query.bindValue(":emotion", static_cast<int>(data.emotionLevel));
    query.bindValue(":pressure", static_cast<int>(data.pressureLevel));
    query.bindValue(":battery", data.battery);
    query.bindValue(":sos", data.sosStatus ? 1 : 0);

    if (!query.exec()) {
        qCritical() << "更新数据失败：" << query.lastError().text();
        return false;
    }
    
    if (query.numRowsAffected() == 0) {
        qWarning() << "未找到ID为" << data.id << "的数据";
        return false;
    }
    
    qDebug() << "更新数据成功，ID：" << data.id;
    return true;
}

HealthData HealthDataManager::getHealthDataById(int id)
{
    HealthData data;
    if (!m_db.isOpen()) {
        return data;
    }

    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT
            id, elder_id, record_time, heart_rate, systolic_pressure, diastolic_pressure,
            temperature, spo2, step_count, emotion_level, pressure_level, battery, sos_status
        FROM health_data
        WHERE id = :id
    )");
    query.bindValue(":id", id);

    if (query.exec() && query.next()) {
        data.id = query.value(0).toInt();
        data.elderId = query.value(1).toString();
        data.recordTime = query.value(2).toDateTime();
        data.heartRate = query.value(3).toInt();
        data.systolicPressure = query.value(4).toInt();
        data.diastolicPressure = query.value(5).toInt();
        data.temperature = query.value(6).toDouble();
        data.spO2 = query.value(7).toInt();
        data.stepCount = query.value(8).toInt();
        data.emotionLevel = static_cast<EmotionLevel>(query.value(9).toInt());
        data.pressureLevel = static_cast<PressureLevel>(query.value(10).toInt());
        data.battery = query.value(11).toInt();
        data.sosStatus = query.value(12).toInt() == 1;
    }
    return data;
}

QVector<HealthData> HealthDataManager::queryHealthData(const QString &elderId, const QDateTime &start, const QDateTime &end)
{
    QVector<HealthData> result;
    if (!m_db.isOpen()) {
        return result;
    }

    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT
            id, elder_id, record_time, heart_rate, systolic_pressure, diastolic_pressure,
            temperature, spo2, step_count, emotion_level, pressure_level, battery, sos_status
        FROM health_data
        WHERE elder_id = :elderId AND record_time BETWEEN :start AND :end
        ORDER BY record_time ASC
    )");

    query.bindValue(":elderId", elderId);
    query.bindValue(":start", start);
    query.bindValue(":end", end);

    if (!query.exec()) {
        qCritical() << "查询数据失败：" << query.lastError().text();
        return result;
    }

    while (query.next()) {
        HealthData data;
        data.id = query.value(0).toInt();
        data.elderId = query.value(1).toString();
        data.recordTime = query.value(2).toDateTime();
        data.heartRate = query.value(3).toInt();
        data.systolicPressure = query.value(4).toInt();
        data.diastolicPressure = query.value(5).toInt();
        data.temperature = query.value(6).toDouble();
        data.spO2 = query.value(7).toInt();
        data.stepCount = query.value(8).toInt();
        data.emotionLevel = static_cast<EmotionLevel>(query.value(9).toInt());
        data.pressureLevel = static_cast<PressureLevel>(query.value(10).toInt());
        data.battery = query.value(11).toInt();
        data.sosStatus = query.value(12).toInt() == 1;

        result.append(data);
    }
    return result;
}

bool HealthDataManager::checkAbnormalData(const HealthData &data)
{
    bool hrAbnormal = (data.heartRate < 60 || data.heartRate > 100);
    bool spAbnormal = (data.systolicPressure > 140);
    bool dpAbnormal = (data.diastolicPressure > 90);
    bool tempAbnormal = (data.temperature < 36.0 || data.temperature > 37.5);
    bool spo2Abnormal = (data.spO2 < 95);
    bool batteryLow = (data.battery < 20);
    bool pressureHigh = (data.pressureLevel >= PRESSURE_HIGH);
    bool sosTriggered = data.sosStatus;

    return hrAbnormal || spAbnormal || dpAbnormal || tempAbnormal ||
           spo2Abnormal || batteryLow || pressureHigh || sosTriggered;
}

QStringList HealthDataManager::getElderList()
{
    QStringList elders;
    if (!m_db.isOpen()) {
        return elders;
    }

    QSqlQuery query(m_db);
    if (query.exec("SELECT DISTINCT elder_id FROM elder_info ORDER BY elder_id")) {
        while (query.next()) {
            elders.append(query.value(0).toString());
        }
    }
    return elders;
}

bool HealthDataManager::addElder(const ElderInfo &info)
{
    if (!m_db.isOpen()) {
        qWarning() << "数据库未打开";
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO elder_info (
            elder_id, name, age, gender, phone, emergency_contact, 
            emergency_phone, address, notes
        ) VALUES (
            :elderId, :name, :age, :gender, :phone, :emergencyContact,
            :emergencyPhone, :address, :notes
        )
    )");

    query.bindValue(":elderId", info.elderId);
    query.bindValue(":name", info.name);
    query.bindValue(":age", info.age);
    query.bindValue(":gender", info.gender);
    query.bindValue(":phone", info.phone);
    query.bindValue(":emergencyContact", info.emergencyContact);
    query.bindValue(":emergencyPhone", info.emergencyPhone);
    query.bindValue(":address", info.address);
    query.bindValue(":notes", info.notes);

    if (!query.exec()) {
        qCritical() << "添加老人失败：" << query.lastError().text();
        return false;
    }
    
    qDebug() << "添加老人成功，老人ID：" << info.elderId;
    return true;
}

bool HealthDataManager::updateElder(const ElderInfo &info)
{
    if (!m_db.isOpen()) {
        qWarning() << "数据库未打开";
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare(R"(
        UPDATE elder_info SET
            name = :name,
            age = :age,
            gender = :gender,
            phone = :phone,
            emergency_contact = :emergencyContact,
            emergency_phone = :emergencyPhone,
            address = :address,
            notes = :notes
        WHERE elder_id = :elderId
    )");

    query.bindValue(":elderId", info.elderId);
    query.bindValue(":name", info.name);
    query.bindValue(":age", info.age);
    query.bindValue(":gender", info.gender);
    query.bindValue(":phone", info.phone);
    query.bindValue(":emergencyContact", info.emergencyContact);
    query.bindValue(":emergencyPhone", info.emergencyPhone);
    query.bindValue(":address", info.address);
    query.bindValue(":notes", info.notes);

    if (!query.exec()) {
        qCritical() << "更新老人信息失败：" << query.lastError().text();
        return false;
    }
    
    qDebug() << "更新老人信息成功，老人ID：" << info.elderId;
    return true;
}

bool HealthDataManager::removeElder(const QString &elderId)
{
    if (!m_db.isOpen()) {
        qWarning() << "数据库未打开";
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare("DELETE FROM elder_info WHERE elder_id = :elderId");
    query.bindValue(":elderId", elderId);

    if (!query.exec()) {
        qCritical() << "删除老人失败：" << query.lastError().text();
        return false;
    }
    
    qDebug() << "删除老人成功，老人ID：" << elderId;
    return true;
}

ElderInfo HealthDataManager::getElderInfo(const QString &elderId)
{
    ElderInfo info;
    if (!m_db.isOpen()) {
        return info;
    }

    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT elder_id, name, age, gender, phone, emergency_contact,
               emergency_phone, address, notes
        FROM elder_info
        WHERE elder_id = :elderId
    )");
    query.bindValue(":elderId", elderId);

    if (query.exec() && query.next()) {
        info.elderId = query.value(0).toString();
        info.name = query.value(1).toString();
        info.age = query.value(2).toInt();
        info.gender = query.value(3).toString();
        info.phone = query.value(4).toString();
        info.emergencyContact = query.value(5).toString();
        info.emergencyPhone = query.value(6).toString();
        info.address = query.value(7).toString();
        info.notes = query.value(8).toString();
    }
    return info;
}

QVector<ElderInfo> HealthDataManager::getAllElders()
{
    QVector<ElderInfo> elders;
    if (!m_db.isOpen()) {
        return elders;
    }

    QSqlQuery query(m_db);
    if (query.exec("SELECT elder_id, name, age, gender, phone, emergency_contact, emergency_phone, address, notes FROM elder_info ORDER BY elder_id")) {
        while (query.next()) {
            ElderInfo info;
            info.elderId = query.value(0).toString();
            info.name = query.value(1).toString();
            info.age = query.value(2).toInt();
            info.gender = query.value(3).toString();
            info.phone = query.value(4).toString();
            info.emergencyContact = query.value(5).toString();
            info.emergencyPhone = query.value(6).toString();
            info.address = query.value(7).toString();
            info.notes = query.value(8).toString();
            elders.append(info);
        }
    }
    return elders;
}

HealthData HealthDataManager::getLatestData(const QString &elderId)
{
    HealthData data;
    if (!m_db.isOpen()) {
        return data;
    }

    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT
            id, elder_id, record_time, heart_rate, systolic_pressure, diastolic_pressure,
            temperature, spo2, step_count, emotion_level, pressure_level, battery, sos_status
        FROM health_data
        WHERE elder_id = :elderId
        ORDER BY record_time DESC
        LIMIT 1
    )");
    query.bindValue(":elderId", elderId);

    if (query.exec() && query.next()) {
        data.id = query.value(0).toInt();
        data.elderId = query.value(1).toString();
        data.recordTime = query.value(2).toDateTime();
        data.heartRate = query.value(3).toInt();
        data.systolicPressure = query.value(4).toInt();
        data.diastolicPressure = query.value(5).toInt();
        data.temperature = query.value(6).toDouble();
        data.spO2 = query.value(7).toInt();
        data.stepCount = query.value(8).toInt();
        data.emotionLevel = static_cast<EmotionLevel>(query.value(9).toInt());
        data.pressureLevel = static_cast<PressureLevel>(query.value(10).toInt());
        data.battery = query.value(11).toInt();
        data.sosStatus = query.value(12).toInt() == 1;
    }
    return data;
}

HealthDataManager::HealthStatistics HealthDataManager::getStatistics(
    const QString &elderId, const QDateTime &start, const QDateTime &end)
{
    HealthStatistics stats;
    if (!m_db.isOpen()) {
        return stats;
    }

    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT
            COUNT(*) as total,
            AVG(heart_rate) as avg_hr,
            MAX(heart_rate) as max_hr,
            MIN(heart_rate) as min_hr,
            AVG(systolic_pressure) as avg_sys,
            AVG(diastolic_pressure) as avg_dia,
            AVG(temperature) as avg_temp,
            AVG(spo2) as avg_spo2
        FROM health_data
        WHERE elder_id = :elderId AND record_time BETWEEN :start AND :end
    )");
    query.bindValue(":elderId", elderId);
    query.bindValue(":start", start);
    query.bindValue(":end", end);

    if (query.exec() && query.next()) {
        stats.totalRecords = query.value(0).toInt();
        stats.avgHeartRate = query.value(1).toDouble();
        stats.maxHeartRate = query.value(2).toDouble();
        stats.minHeartRate = query.value(3).toDouble();
        stats.avgSystolic = query.value(4).toDouble();
        stats.avgDiastolic = query.value(5).toDouble();
        stats.avgTemperature = query.value(6).toDouble();
        stats.avgSpO2 = query.value(7).toDouble();

        QVector<HealthData> allData = queryHealthData(elderId, start, end);
        for (const HealthData &d : allData) {
            if (checkAbnormalData(d)) {
                stats.abnormalCount++;
            }
        }
    }
    return stats;
}

bool HealthDataManager::cleanOldData(int keepDays)
{
    if (!m_db.isOpen()) {
        return false;
    }

    QDateTime cutoffDate = QDateTime::currentDateTime().addDays(-keepDays);
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM health_data WHERE record_time < :cutoff");
    query.bindValue(":cutoff", cutoffDate);

    if (!query.exec()) {
        qCritical() << "清理过期数据失败：" << query.lastError().text();
        return false;
    }
    
    qDebug() << "清理过期数据完成，删除" << query.numRowsAffected() << "条记录";
    return true;
}

bool HealthDataManager::exportToCSV(const QString &elderId, const QDateTime &start,
                                     const QDateTime &end, const QString &filePath)
{
    QVector<HealthData> dataList = queryHealthData(elderId, start, end);
    if (dataList.isEmpty()) {
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    out.setEncoding(QStringConverter::Utf8);
#else
    out.setCodec("UTF-8");
#endif
    out << "ID,老人ID,记录时间,心率,收缩压,舒张压,体温,血氧,步数,情绪等级,压力等级,电量,SOS状态\n";

    for (const HealthData &data : dataList) {
        out << data.id << ","
            << data.elderId << ","
            << data.recordTime.toString("yyyy-MM-dd HH:mm:ss") << ","
            << data.heartRate << ","
            << data.systolicPressure << ","
            << data.diastolicPressure << ","
            << data.temperature << ","
            << data.spO2 << ","
            << data.stepCount << ","
            << static_cast<int>(data.emotionLevel) << ","
            << static_cast<int>(data.pressureLevel) << ","
            << data.battery << ","
            << (data.sosStatus ? "是" : "否") << "\n";
    }

    file.close();
    emit dataImported(elderId, dataList.size());
    return true;
}
