#include "serialdevice.h"
#include <QDebug>

SerialDevice::SerialDevice(QObject *parent) : QObject(parent)
{
    m_serialPort = new QSerialPort(this);
    connect(m_serialPort, &QSerialPort::readyRead, this, &SerialDevice::readSerialData);
    connect(m_serialPort, QOverload<QSerialPort::SerialPortError>::of(&QSerialPort::errorOccurred),
            this, &SerialDevice::onSerialError);

    m_workerThread = new QThread(this);
    m_worker = new ThreadWorker();
    m_worker->moveToThread(m_workerThread);

    connect(this, &SerialDevice::sendDataToWorker, m_worker, &ThreadWorker::doParseAndSave);
    connect(m_worker, &ThreadWorker::dataParsed, this, &SerialDevice::healthDataReceived);
    connect(m_worker, &ThreadWorker::sosParsed, this, &SerialDevice::sosReceived);
    connect(m_workerThread, &QThread::finished, m_worker, &ThreadWorker::deleteLater);
    connect(m_workerThread, &QThread::finished, m_workerThread, &QThread::deleteLater);

    m_workerThread->start();

    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setInterval(m_reconnectInterval);
    connect(m_reconnectTimer, &QTimer::timeout, this, &SerialDevice::tryReconnect);
}

SerialDevice::~SerialDevice()
{
    closeSerialPort();
    if (m_workerThread->isRunning()) {
        m_workerThread->quit();
        m_workerThread->wait();
    }
}

bool SerialDevice::isPortOpen() const
{
    return m_serialPort->isOpen();
}

bool SerialDevice::openSerialPort(const QString &portName, qint32 baudRate)
{
    m_portName = portName;
    m_baudRate = baudRate;
    
    m_serialPort->setPortName(portName);
    m_serialPort->setBaudRate(baudRate);
    m_serialPort->setDataBits(QSerialPort::Data8);
    m_serialPort->setParity(QSerialPort::NoParity);
    m_serialPort->setStopBits(QSerialPort::OneStop);
    m_serialPort->setFlowControl(QSerialPort::NoFlowControl);

    bool isOpen = m_serialPort->open(QIODevice::ReadWrite);
    if (isOpen) {
        qDebug() << "串口打开成功：" << portName;
        m_reconnectTimer->stop();
        emit serialPortConnected();
    } else {
        qDebug() << "串口打开失败：" << m_serialPort->errorString();
        if (m_autoReconnect) {
            m_reconnectTimer->start();
        }
    }
    return isOpen;
}

void SerialDevice::closeSerialPort()
{
    m_reconnectTimer->stop();
    
    if (m_serialPort->isOpen()) {
        m_serialPort->close();
        qDebug() << "串口已关闭";
        emit serialPortDisconnected();
    }
}

void SerialDevice::readSerialData()
{
    QByteArray data = m_serialPort->readAll();
    if (data.isEmpty()) {
        return;
    }

    m_receiveBuffer.append(data);
    
    if (m_receiveBuffer.size() > MAX_BUFFER_SIZE) {
        int newlinePos = m_receiveBuffer.indexOf('\n');
        if (newlinePos >= 0) {
            m_receiveBuffer = m_receiveBuffer.mid(newlinePos + 1);
        } else {
            m_receiveBuffer.clear();
        }
        qWarning() << "串口接收缓冲区溢出，已清理";
    }

    processBuffer();
}

void SerialDevice::processBuffer()
{
    while (true) {
        int newlinePos = m_receiveBuffer.indexOf('\n');
        if (newlinePos < 0) {
            break;
        }

        QByteArray line = m_receiveBuffer.left(newlinePos).trimmed();
        m_receiveBuffer = m_receiveBuffer.mid(newlinePos + 1);

        if (!line.isEmpty()) {
            qDebug() << "接收串口数据：" << line;
            emit sendDataToWorker(line);
        }
    }
}

void SerialDevice::onSerialError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError) {
        return;
    }

    QString errorStr = m_serialPort->errorString();
    qWarning() << "串口错误：" << errorStr;
    emit serialPortError(errorStr);

    if (error == QSerialPort::ResourceError || error == QSerialPort::TimeoutError) {
        m_serialPort->close();
        emit serialPortDisconnected();
        
        if (m_autoReconnect) {
            qDebug() << "将在" << m_reconnectInterval << "ms后尝试重连...";
            m_reconnectTimer->start();
        }
    }
}

void SerialDevice::tryReconnect()
{
    if (m_serialPort->isOpen()) {
        m_reconnectTimer->stop();
        return;
    }

    qDebug() << "尝试重新连接串口：" << m_portName;
    if (openSerialPort(m_portName, m_baudRate)) {
        m_reconnectTimer->stop();
    }
}

void SerialDevice::setHealthDataManager(HealthDataManager *manager)
{
    if (m_worker) {
        m_worker->setHealthDataManager(manager);
    }
}
