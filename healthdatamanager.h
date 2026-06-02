#ifndef HEALTHDATAMANAGER_H
#define HEALTHDATAMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QDateTime>
#include <QVector>
#include <QStringList>

// 情绪/压力等级枚举
enum EmotionLevel {
    EMOTION_HAPPY = 0,    // 开心
    EMOTION_NORMAL = 1,   // 正常
    EMOTION_ANXIOUS = 2,  // 焦虑
    EMOTION_SAD = 3,      // 低落
    EMOTION_ANGRY = 4     // 愤怒
};

enum PressureLevel {
    PRESSURE_NONE = 0,    // 无压力
    PRESSURE_LOW = 1,     // 轻度
    PRESSURE_MID = 2,     // 中度
    PRESSURE_HIGH = 3,    // 重度
    PRESSURE_EXTREME = 4  // 极度
};

// 健康数据结构体（完整版）
struct HealthData {
    int id = 0;                      // 主键
    QString elderId;                 // 老人ID
    QDateTime recordTime;            // 记录时间

    // 基础生理指标
    int heartRate = 0;               // 心率（次/分钟）
    int systolicPressure = 0;        // 收缩压（mmHg）
    int diastolicPressure = 0;       // 舒张压（mmHg）
    double temperature = 0.0;        // 体温（℃）
    int spO2 = 0;                    // 血氧饱和度（%）

    // 行为/状态指标
    int stepCount = 0;               // 今日步数
    EmotionLevel emotionLevel = EMOTION_NORMAL; // 情绪等级
    PressureLevel pressureLevel = PRESSURE_NONE; // 压力等级
    int battery = 100;               // 手环电量（%）
    bool sosStatus = false;          // SOS求助状态
};

struct ElderInfo {
    QString elderId;                 // 老人ID
    QString name;                    // 姓名
    int age = 0;                     // 年龄
    QString gender;                  // 性别
    QString phone;                   // 联系电话
    QString emergencyContact;        // 紧急联系人
    QString emergencyPhone;          // 紧急联系人电话
    QString address;                 // 地址
    QString notes;                   // 备注
};

class HealthDataManager : public QObject
{
    Q_OBJECT
public:
    explicit HealthDataManager(QObject *parent = nullptr);
    ~HealthDataManager() override;

    // 初始化数据库
    bool initDatabase();
    
    // 【增】插入健康数据
    bool insertHealthData(const HealthData &data);
    // 【增】批量插入健康数据
    bool insertHealthDataBatch(const QVector<HealthData> &dataList);
    
    // 【删】按ID删除数据
    bool deleteHealthDataById(int id);
    // 【删】按老人ID删除所有数据
    bool deleteHealthDataByElderId(const QString &elderId);
    // 【删】清理过期数据（保留天数）
    bool cleanOldData(int keepDays = 30);
    
    // 【改】更新健康数据
    bool updateHealthData(const HealthData &data);
    
    // 【查】按ID查询数据
    HealthData getHealthDataById(int id);
    // 【查】查询指定老人的健康数据
    QVector<HealthData> queryHealthData(const QString &elderId, const QDateTime &start, const QDateTime &end);
    // 【查】获取最新一条数据
    HealthData getLatestData(const QString &elderId);
    // 【查】获取老人列表
    QStringList getElderList();
    // 【老人管理】添加老人
    bool addElder(const ElderInfo &info);
    // 【老人管理】更新老人信息
    bool updateElder(const ElderInfo &info);
    // 【老人管理】删除老人
    bool removeElder(const QString &elderId);
    // 【老人管理】获取老人信息
    ElderInfo getElderInfo(const QString &elderId);
    // 【老人管理】获取所有老人信息
    QVector<ElderInfo> getAllElders();
    // 【查】获取统计数据
    struct HealthStatistics {
        double avgHeartRate = 0;
        double maxHeartRate = 0;
        double minHeartRate = 0;
        double avgSystolic = 0;
        double avgDiastolic = 0;
        double avgTemperature = 0;
        double avgSpO2 = 0;
        int totalRecords = 0;
        int abnormalCount = 0;
    };
    HealthStatistics getStatistics(const QString &elderId, const QDateTime &start, const QDateTime &end);
    
    // 检测数据是否异常
    bool checkAbnormalData(const HealthData &data);
    // 导出数据为CSV
    bool exportToCSV(const QString &elderId, const QDateTime &start, const QDateTime &end, const QString &filePath);

signals:
    void dataImported(const QString &elderId, int count);

private:
    QSqlDatabase m_db;
    bool createIndexes();
};

#endif // HEALTHDATAMANAGER_H
