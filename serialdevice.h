#ifndef SERIALDEVICE_H
#define SERIALDEVICE_H

#include <QObject>
#include <QSerialPort>
#include <QThread>
#include <QTimer>
#include <QByteArray>
#include "threadworker.h"
#include "healthdatamanager.h"

class SerialDevice : public QObject
{
    Q_OBJECT
public:
    explicit SerialDevice(QObject *parent = nullptr);
    ~SerialDevice() override;

    bool openSerialPort(const QString &portName, qint32 baudRate = 9600);
    void closeSerialPort();
    bool isPortOpen() const;
    QString getPortName() const { return m_portName; }
    qint32 getBaudRate() const { return m_baudRate; }
    
    void setAutoReconnect(bool enable) { m_autoReconnect = enable; }
    void setReconnectInterval(int ms) { m_reconnectInterval = ms; }
    void setHealthDataManager(HealthDataManager *manager);

signals:
    void sendDataToWorker(const QByteArray &data);
    void healthDataReceived(const QString &elderId, const HealthData &data);
    void sosReceived(const QString &elderId);
    void serialPortError(const QString &error);
    void serialPortConnected();
    void serialPortDisconnected();

private slots:
    void readSerialData();
    void onSerialError(QSerialPort::SerialPortError error);
    void tryReconnect();

private:
    void processBuffer();
    
    QSerialPort *m_serialPort;
    QThread *m_workerThread;
    ThreadWorker *m_worker;
    QTimer *m_reconnectTimer;
    
    QByteArray m_receiveBuffer;
    QString m_portName;
    qint32 m_baudRate = 9600;
    bool m_autoReconnect = true;
    int m_reconnectInterval = 5000;
    static const int MAX_BUFFER_SIZE = 4096;
};

#endif
