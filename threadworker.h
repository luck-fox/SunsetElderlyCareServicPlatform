#ifndef THREADWORKER_H
#define THREADWORKER_H

#include <QObject>
#include <QByteArray>
#include <QStringList>
#include "healthdatamanager.h"

class ThreadWorker : public QObject
{
    Q_OBJECT
public:
    explicit ThreadWorker(QObject *parent = nullptr);
    void setHealthDataManager(HealthDataManager *manager);

signals:
    void dataParsed(const QString &elderId, const HealthData &data);
    void sosParsed(const QString &elderId);
    void dataSaved(bool ok);
    void parseError(const QString &error);

public slots:
    void doParseAndSave(const QByteArray &data);

private:
    bool validateData(const HealthData &data, QStringList &errors);
    
    HealthDataManager *m_healthMgr = nullptr;
    bool m_useExternalManager = false;
};

#endif
