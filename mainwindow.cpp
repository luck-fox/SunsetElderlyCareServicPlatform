#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QChart>
#include <QLineSeries>
#include <QDateTimeAxis>
#include <QValueAxis>
#include <QPainter>
#include <QRandomGenerator>
#include <QMessageBox>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QComboBox>
#include <QPushButton>
#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QTextEdit>
#include <QTableWidget>
#include <QHeaderView>
#include <QDebug>
#include <QMouseEvent>
#include <QLegend>
#include <QAbstractSeries>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_labelHr(nullptr)
    , m_labelBp(nullptr)
    , m_labelTemp(nullptr)
    , m_labelSpO2(nullptr)
    , m_labelStep(nullptr)
    , m_labelEmotion(nullptr)
    , m_labelPressure(nullptr)
    , m_labelBattery(nullptr)
    , m_labelSOS(nullptr)
    , m_labelTime(nullptr)
    , m_labelStats(nullptr)
    , m_comboElder(nullptr)
    , m_btnToggleTest(nullptr)
    , m_btnExport(nullptr)
    , m_btnStats(nullptr)
    , m_btnClean(nullptr)
    , m_chartViewHr(nullptr)
    , m_chartViewBp(nullptr)
    , m_chartViewTemp(nullptr)
    , m_chartViewSpO2(nullptr)
    , m_hrChart(nullptr)
    , m_bpChart(nullptr)
    , m_tempChart(nullptr)
    , m_spO2Chart(nullptr)
    , m_healthMgr(nullptr)
    , m_serialDevice(nullptr)
    , m_httpPush(nullptr)
    , m_testDataTimer(nullptr)
    , m_chartUpdateTimer(nullptr)
    , m_isTestDataRunning(true)
{
    qDebug() << "MainWindow构造函数开始...";
    resize(1400, 900);
    setWindowTitle("智慧夕阳服务管理系统");

    qDebug() << "初始化HealthDataManager...";
    m_healthMgr = new HealthDataManager(this);
    
    if (!m_healthMgr->initDatabase()) {
        qDebug() << "数据库初始化失败";
        QMessageBox::critical(this, "数据库错误", "数据库初始化失败，程序将退出。");
        return;
    }
    qDebug() << "数据库初始化成功";

    qDebug() << "初始化SerialDevice...";
    m_serialDevice = new SerialDevice(this);
    m_httpPush = new HttpPushService(this);

    m_serialDevice->setHealthDataManager(m_healthMgr);

    qDebug() << "设置UI...";
    setupUI();
    setupMenuBar();

    connect(m_serialDevice, &SerialDevice::healthDataReceived,
            this, &MainWindow::onHealthDataReceived);
    connect(m_serialDevice, &SerialDevice::sosReceived,
            this, &MainWindow::onSOSReceived);

    m_testDataTimer = new QTimer(this);
    connect(m_testDataTimer, &QTimer::timeout, this, [=]() {
        if (!m_isTestDataRunning) return;
        
        HealthData data;
        data.elderId = m_currentElderId;
        data.recordTime = QDateTime::currentDateTime();
        data.heartRate = QRandomGenerator::global()->bounded(60, 100);
        data.systolicPressure = QRandomGenerator::global()->bounded(100, 140);
        data.diastolicPressure = QRandomGenerator::global()->bounded(70, 90);
        data.temperature = QRandomGenerator::global()->bounded(360, 375) / 10.0;
        data.spO2 = QRandomGenerator::global()->bounded(95, 100);
        data.stepCount = QRandomGenerator::global()->bounded(0, 5000);
        data.emotionLevel = static_cast<EmotionLevel>(QRandomGenerator::global()->bounded(0, 5));
        data.pressureLevel = static_cast<PressureLevel>(QRandomGenerator::global()->bounded(0, 5));
        data.battery = QRandomGenerator::global()->bounded(20, 100);
        data.sosStatus = false;

        if (QRandomGenerator::global()->bounded(0, 100) == 50) {
            data.sosStatus = true;
        }

        onHealthDataReceived(data.elderId, data);
    });

    m_chartUpdateTimer = new QTimer(this);
    connect(m_chartUpdateTimer, &QTimer::timeout, this, &MainWindow::updateCharts);
    m_chartUpdateTimer->start(5000);

    QStringList elders = m_healthMgr->getElderList();
    if (elders.isEmpty()) {
        m_currentElderId = "LAO001";
        m_comboElder->addItem("LAO001");
    } else {
        m_currentElderId = elders.first();
        m_comboElder->addItems(elders);
    }

    QTimer::singleShot(1000, this, [=]() {
        drawHealthChart(m_currentElderId);
    });
    
    QTimer::singleShot(3000, this, [=]() {
        m_testDataTimer->start(3000);
    });
    
    qDebug() << "MainWindow构造函数完成";
}

MainWindow::~MainWindow()
{
    m_testDataTimer->stop();
    m_chartUpdateTimer->stop();
    m_serialDevice->closeSerialPort();
}

void MainWindow::setupUI()
{
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    setCentralWidget(centralWidget);

    QWidget *controlBar = new QWidget(centralWidget);
    QHBoxLayout *controlLayout = new QHBoxLayout(controlBar);
    controlLayout->setSpacing(10);
    
    QLabel *labelElder = new QLabel("选择老人：", controlBar);
    labelElder->setStyleSheet("font-size: 14px; font-weight: bold;");
    controlLayout->addWidget(labelElder);
    
    m_comboElder = new QComboBox(controlBar);
    m_comboElder->setMinimumWidth(150);
    m_comboElder->setStyleSheet(
        "QComboBox {"
        "    padding: 5px;"
        "    border: 2px solid #4CAF50;"
        "    border-radius: 5px;"
        "    font-size: 14px;"
        "}"
        "QComboBox::drop-down {"
        "    subcontrol-origin: padding;"
        "    subcontrol-position: top right;"
        "    width: 20px;"
        "    border-left: 2px solid #4CAF50;"
        "}"
    );
    controlLayout->addWidget(m_comboElder);
    
    m_btnToggleTest = new QPushButton("停止模拟数据", controlBar);
    m_btnToggleTest->setStyleSheet(
        "QPushButton {"
        "    background-color: #FF9800;"
        "    color: white;"
        "    border: none;"
        "    padding: 8px 16px;"
        "    border-radius: 5px;"
        "    font-size: 14px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: #F57C00;"
        "}"
    );
    controlLayout->addWidget(m_btnToggleTest);
    
    m_btnExport = new QPushButton("导出数据", controlBar);
    m_btnExport->setStyleSheet(
        "QPushButton {"
        "    background-color: #2196F3;"
        "    color: white;"
        "    border: none;"
        "    padding: 8px 16px;"
        "    border-radius: 5px;"
        "    font-size: 14px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: #1976D2;"
        "}"
    );
    controlLayout->addWidget(m_btnExport);
    
    m_btnStats = new QPushButton("查看统计", controlBar);
    m_btnStats->setStyleSheet(
        "QPushButton {"
        "    background-color: #9C27B0;"
        "    color: white;"
        "    border: none;"
        "    padding: 8px 16px;"
        "    border-radius: 5px;"
        "    font-size: 14px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: #7B1FA2;"
        "}"
    );
    controlLayout->addWidget(m_btnStats);
    
    m_btnClean = new QPushButton("清理旧数据", controlBar);
    m_btnClean->setStyleSheet(
        "QPushButton {"
        "    background-color: #F44336;"
        "    color: white;"
        "    border: none;"
        "    padding: 8px 16px;"
        "    border-radius: 5px;"
        "    font-size: 14px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: #D32F2F;"
        "}"
    );
    controlLayout->addWidget(m_btnClean);
    
    m_btnAddElder = new QPushButton("添加老人", controlBar);
    m_btnAddElder->setStyleSheet(
        "QPushButton {"
        "    background-color: #4CAF50;"
        "    color: white;"
        "    border: none;"
        "    padding: 8px 16px;"
        "    border-radius: 5px;"
        "    font-size: 14px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: #45a049;"
        "}"
    );
    controlLayout->addWidget(m_btnAddElder);
    
    m_btnManageElders = new QPushButton("管理老人", controlBar);
    m_btnManageElders->setStyleSheet(
        "QPushButton {"
        "    background-color: #009688;"
        "    color: white;"
        "    border: none;"
        "    padding: 8px 16px;"
        "    border-radius: 5px;"
        "    font-size: 14px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: #00796b;"
        "}"
    );
    controlLayout->addWidget(m_btnManageElders);
    
    controlLayout->addStretch();
    
    mainLayout->addWidget(controlBar);

    connect(m_comboElder, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int index) {
        if (m_comboElder) {
            onElderChanged(m_comboElder->itemText(index));
        }
    });
    connect(m_btnToggleTest, &QPushButton::clicked, this, &MainWindow::onToggleTestData);
    connect(m_btnExport, &QPushButton::clicked, this, &MainWindow::onExportData);
    connect(m_btnStats, &QPushButton::clicked, this, &MainWindow::onViewStatistics);
    connect(m_btnClean, &QPushButton::clicked, this, &MainWindow::onCleanOldData);
    connect(m_btnAddElder, &QPushButton::clicked, this, &MainWindow::onAddElder);
    connect(m_btnManageElders, &QPushButton::clicked, this, &MainWindow::onManageElders);

    QGroupBox *dataGroup = new QGroupBox("实时健康数据", centralWidget);
    dataGroup->setStyleSheet(
        "QGroupBox {"
        "    font-size: 16px;"
        "    font-weight: bold;"
        "    border: 2px solid #4CAF50;"
        "    border-radius: 8px;"
        "    margin-top: 10px;"
        "    padding-top: 20px;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    left: 10px;"
        "    padding: 0 5px;"
        "    color: #4CAF50;"
        "}"
    );
    
    QGridLayout *dataLayout = new QGridLayout(dataGroup);
    dataLayout->setSpacing(15);
    dataLayout->setContentsMargins(15, 15, 15, 15);

    m_labelHr = new QLabel("心率：-- 次/分", dataGroup);
    m_labelBp = new QLabel("血压：--/-- mmHg", dataGroup);
    m_labelTemp = new QLabel("体温：-- ℃", dataGroup);
    m_labelSpO2 = new QLabel("血氧：-- %", dataGroup);
    m_labelStep = new QLabel("今日步数：-- 步", dataGroup);
    m_labelEmotion = new QLabel("情绪状态：--", dataGroup);
    m_labelPressure = new QLabel("压力等级：--", dataGroup);
    m_labelBattery = new QLabel("手环电量：-- %", dataGroup);
    m_labelSOS = new QLabel("SOS状态：正常", dataGroup);
    m_labelTime = new QLabel("更新时间：--", dataGroup);
    m_labelStats = new QLabel("统计：--", dataGroup);

    QFont labelFont("微软雅黑", 14);
    for (QLabel *label : {m_labelHr, m_labelBp, m_labelTemp, m_labelSpO2, 
                          m_labelStep, m_labelEmotion, m_labelPressure, 
                          m_labelBattery, m_labelSOS, m_labelTime, m_labelStats}) {
        label->setFont(labelFont);
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet(
            "QLabel {"
            "    background-color: #F5F5F5;"
            "    border: 2px solid #E0E0E0;"
            "    border-radius: 5px;"
            "    padding: 10px;"
            "}"
        );
    }
    m_labelSOS->setStyleSheet(
        "QLabel {"
        "    background-color: #E8F5E9;"
        "    border: 2px solid #4CAF50;"
        "    border-radius: 5px;"
        "    padding: 10px;"
        "    color: #4CAF50;"
        "    font-weight: bold;"
        "}"
    );

    dataLayout->addWidget(m_labelHr, 0, 0);
    dataLayout->addWidget(m_labelBp, 0, 1);
    dataLayout->addWidget(m_labelTemp, 0, 2);
    dataLayout->addWidget(m_labelSpO2, 1, 0);
    dataLayout->addWidget(m_labelStep, 1, 1);
    dataLayout->addWidget(m_labelEmotion, 1, 2);
    dataLayout->addWidget(m_labelPressure, 2, 0);
    dataLayout->addWidget(m_labelBattery, 2, 1);
    dataLayout->addWidget(m_labelSOS, 2, 2);
    dataLayout->addWidget(m_labelTime, 3, 0, 1, 3);
    dataLayout->addWidget(m_labelStats, 4, 0, 1, 3);

    mainLayout->addWidget(dataGroup);

    QWidget *chartWidget = new QWidget(centralWidget);
    QGridLayout *chartLayout = new QGridLayout(chartWidget);
    chartLayout->setSpacing(10);
    chartLayout->setContentsMargins(0, 0, 0, 0);

    m_chartViewHr = new ClickableChartView(0, chartWidget);
    m_chartViewBp = new ClickableChartView(1, chartWidget);
    m_chartViewTemp = new ClickableChartView(2, chartWidget);
    m_chartViewSpO2 = new ClickableChartView(3, chartWidget);

    for (QChartView *view : {m_chartViewHr, m_chartViewBp, m_chartViewTemp, m_chartViewSpO2}) {
        view->setRenderHint(QPainter::Antialiasing);
        view->setToolTip("双击可全屏查看趋势图");
        view->setStyleSheet(
            "QChartView {"
            "    border: 2px solid #2196F3;"
            "    border-radius: 8px;"
            "    background-color: white;"
            "}"
        );
    }

    chartLayout->addWidget(m_chartViewHr, 0, 0);
    chartLayout->addWidget(m_chartViewBp, 0, 1);
    chartLayout->addWidget(m_chartViewTemp, 1, 0);
    chartLayout->addWidget(m_chartViewSpO2, 1, 1);

    connect(m_chartViewHr, &ClickableChartView::doubleClicked, this, &MainWindow::onChartDoubleClicked);
    connect(m_chartViewBp, &ClickableChartView::doubleClicked, this, &MainWindow::onChartDoubleClicked);
    connect(m_chartViewTemp, &ClickableChartView::doubleClicked, this, &MainWindow::onChartDoubleClicked);
    connect(m_chartViewSpO2, &ClickableChartView::doubleClicked, this, &MainWindow::onChartDoubleClicked);

    mainLayout->addWidget(chartWidget);

    m_hrChart = nullptr;
    m_bpChart = nullptr;
    m_tempChart = nullptr;
    m_spO2Chart = nullptr;
}

void MainWindow::setupMenuBar()
{
    QMenu *fileMenu = menuBar()->addMenu("文件");
    
    QAction *exportAction = fileMenu->addAction("导出数据");
    connect(exportAction, &QAction::triggered, this, &MainWindow::onExportData);
    
    QAction *cleanAction = fileMenu->addAction("清理旧数据");
    connect(cleanAction, &QAction::triggered, this, &MainWindow::onCleanOldData);
    
    fileMenu->addSeparator();
    QAction *exitAction = fileMenu->addAction("退出");
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    QMenu *viewMenu = menuBar()->addMenu("查看");
    QAction *statsAction = viewMenu->addAction("查看统计");
    connect(statsAction, &QAction::triggered, this, &MainWindow::onViewStatistics);

    QMenu *toolMenu = menuBar()->addMenu("工具");
    QAction *toggleAction = toolMenu->addAction("切换模拟数据");
    connect(toggleAction, &QAction::triggered, this, &MainWindow::onToggleTestData);
}

void MainWindow::onHealthDataReceived(const QString &elderId, const HealthData &data)
{
    if (!m_healthMgr) {
        qWarning() << "HealthDataManager未初始化";
        return;
    }
    
    updateDataDisplay(data);
    m_healthMgr->insertHealthData(data);

    if (m_healthMgr->checkAbnormalData(data)) {
        if (m_httpPush) {
            m_httpPush->pushAlert("13800138000", "张大爷", data);
        }
    } else {
        if (m_httpPush) {
            m_httpPush->pushReport("13800138000", "张大爷", data);
        }
    }
}

void MainWindow::updateDataDisplay(const HealthData &data)
{
    if (!m_labelHr || !m_labelBp || !m_labelTemp || !m_labelSpO2 ||
        !m_labelStep || !m_labelEmotion || !m_labelPressure ||
        !m_labelBattery || !m_labelSOS || !m_labelTime || !m_labelStats) {
        qWarning() << "UI标签未初始化";
        return;
    }
    
    m_labelHr->setText(QString("心率：%1 次/分").arg(data.heartRate));
    m_labelBp->setText(QString("血压：%1/%2 mmHg").arg(data.systolicPressure).arg(data.diastolicPressure));
    m_labelTemp->setText(QString("体温：%1 ℃").arg(data.temperature, 0, 'f', 1));
    m_labelSpO2->setText(QString("血氧：%1 %").arg(data.spO2));

    if (m_healthMgr) {
        HealthData latestData = m_healthMgr->getLatestData(data.elderId);
        m_labelStep->setText(QString("今日步数：%1 步").arg(latestData.stepCount));
    }

    QString emotionStr;
    switch (data.emotionLevel) {
    case EMOTION_HAPPY: emotionStr = "开心"; break;
    case EMOTION_NORMAL: emotionStr = "正常"; break;
    case EMOTION_ANXIOUS: emotionStr = "焦虑"; break;
    case EMOTION_SAD: emotionStr = "低落"; break;
    case EMOTION_ANGRY: emotionStr = "愤怒"; break;
    default: emotionStr = "未知";
    }
    m_labelEmotion->setText(QString("情绪状态：%1").arg(emotionStr));

    QString pressureStr;
    switch (data.pressureLevel) {
    case PRESSURE_NONE: pressureStr = "无压力"; break;
    case PRESSURE_LOW: pressureStr = "轻度"; break;
    case PRESSURE_MID: pressureStr = "中度"; break;
    case PRESSURE_HIGH: pressureStr = "重度"; break;
    case PRESSURE_EXTREME: pressureStr = "极度"; break;
    default: pressureStr = "未知";
    }
    m_labelPressure->setText(QString("压力等级：%1").arg(pressureStr));

    m_labelBattery->setText(QString("手环电量：%1 %").arg(data.battery));

    if (data.sosStatus) {
        m_labelSOS->setText("SOS状态：紧急求助！");
        m_labelSOS->setStyleSheet(
            "QLabel {"
            "    background-color: #FFEBEE;"
            "    border: 2px solid #F44336;"
            "    border-radius: 5px;"
            "    padding: 10px;"
            "    color: #F44336;"
            "    font-weight: bold;"
            "}"
        );
    } else {
        m_labelSOS->setText("SOS状态：正常");
        m_labelSOS->setStyleSheet(
            "QLabel {"
            "    background-color: #E8F5E9;"
            "    border: 2px solid #4CAF50;"
            "    border-radius: 5px;"
            "    padding: 10px;"
            "    color: #4CAF50;"
            "    font-weight: bold;"
            "}"
        );
    }

    m_labelTime->setText(QString("更新时间：%1").arg(data.recordTime.toString("yyyy-MM-dd HH:mm:ss")));

    updateStatisticsDisplay();
}

void MainWindow::updateStatisticsDisplay()
{
    QDateTime todayStart = QDateTime::currentDateTime().date().startOfDay();
    HealthDataManager::HealthStatistics stats = 
        m_healthMgr->getStatistics(m_currentElderId, todayStart, QDateTime::currentDateTime());
    
    if (stats.totalRecords > 0) {
        m_labelStats->setText(
            QString("今日统计：记录%1条 | 异常%2条 | 平均心率%3 | 平均血压%4/%5")
                .arg(stats.totalRecords)
                .arg(stats.abnormalCount)
                .arg(stats.avgHeartRate, 0, 'f', 1)
                .arg(stats.avgSystolic, 0, 'f', 1)
                .arg(stats.avgDiastolic, 0, 'f', 1)
        );
    }
}

void MainWindow::drawHealthChart(const QString &elderId)
{
    qDebug() << "drawHealthChart被调用，elderId:" << elderId;
    
    if (!m_healthMgr || !m_chartViewHr || !m_chartViewBp || 
        !m_chartViewTemp || !m_chartViewSpO2) {
        qWarning() << "图表组件未初始化";
        return;
    }
    
    QDateTime startTime = QDateTime::currentDateTime().date().startOfDay();
    QDateTime endTime = QDateTime::currentDateTime();

    QVector<HealthData> dataList = m_healthMgr->queryHealthData(elderId, startTime, endTime);
    qDebug() << "查询到数据条数:" << dataList.size();

    if (dataList.isEmpty()) {
        qDebug() << "数据为空，显示空图表";
        QChart *hrChart = new QChart();
        hrChart->setTitle("暂无数据，等待3秒后生成模拟数据...");
        hrChart->setTitleFont(QFont("微软雅黑", 14));
        m_chartViewHr->setChart(hrChart);

        QChart *bpChart = new QChart();
        bpChart->setTitle("暂无数据，等待3秒后生成模拟数据...");
        bpChart->setTitleFont(QFont("微软雅黑", 14));
        m_chartViewBp->setChart(bpChart);

        QChart *tempChart = new QChart();
        tempChart->setTitle("暂无数据，等待3秒后生成模拟数据...");
        tempChart->setTitleFont(QFont("微软雅黑", 14));
        m_chartViewTemp->setChart(tempChart);

        QChart *spo2Chart = new QChart();
        spo2Chart->setTitle("暂无数据，等待3秒后生成模拟数据...");
        spo2Chart->setTitleFont(QFont("微软雅黑", 14));
        m_chartViewSpO2->setChart(spo2Chart);
        qDebug() << "空图表设置完成";
        return;
    }

    qDebug() << "开始创建心率图表...";
    QLineSeries *hrSeries = new QLineSeries();
    hrSeries->setName("心率(次/分钟)");
    for (const HealthData &data : dataList) {
        hrSeries->append(data.recordTime.toMSecsSinceEpoch(), data.heartRate);
    }

    m_hrChart = new QChart();
    m_hrChart->addSeries(hrSeries);
    m_hrChart->setTitle("今日心率趋势图");
    m_hrChart->setTitleFont(QFont("微软雅黑", 16));
    m_hrChart->legend()->setFont(QFont("微软雅黑", 14));

    QDateTimeAxis *hrXAxis = new QDateTimeAxis();
    hrXAxis->setFormat("HH:mm");
    hrXAxis->setTitleText("今日时间");
    hrXAxis->setTitleFont(QFont("微软雅黑", 14));
    hrXAxis->setRange(startTime, endTime);
    m_hrChart->addAxis(hrXAxis, Qt::AlignBottom);
    hrSeries->attachAxis(hrXAxis);

    QValueAxis *hrYAxis = new QValueAxis();
    hrYAxis->setRange(40, 120);
    hrYAxis->setTitleText("心率");
    hrYAxis->setTitleFont(QFont("微软雅黑", 14));
    m_hrChart->addAxis(hrYAxis, Qt::AlignLeft);
    hrSeries->attachAxis(hrYAxis);

    m_chartViewHr->setChart(m_hrChart);
    qDebug() << "心率图表创建完成";

    qDebug() << "开始创建血压图表...";
    QLineSeries *spSeries = new QLineSeries();
    spSeries->setName("收缩压(mmHg)");
    QLineSeries *dpSeries = new QLineSeries();
    dpSeries->setName("舒张压(mmHg)");

    for (const HealthData &data : dataList) {
        spSeries->append(data.recordTime.toMSecsSinceEpoch(), data.systolicPressure);
        dpSeries->append(data.recordTime.toMSecsSinceEpoch(), data.diastolicPressure);
    }

    m_bpChart = new QChart();
    m_bpChart->addSeries(spSeries);
    m_bpChart->addSeries(dpSeries);
    m_bpChart->setTitle("今日血压趋势图");
    m_bpChart->setTitleFont(QFont("微软雅黑", 16));
    m_bpChart->legend()->setFont(QFont("微软雅黑", 14));

    QDateTimeAxis *bpXAxis = new QDateTimeAxis();
    bpXAxis->setFormat("HH:mm");
    bpXAxis->setTitleText("今日时间");
    bpXAxis->setTitleFont(QFont("微软雅黑", 14));
    bpXAxis->setRange(startTime, endTime);
    m_bpChart->addAxis(bpXAxis, Qt::AlignBottom);
    spSeries->attachAxis(bpXAxis);
    dpSeries->attachAxis(bpXAxis);

    QValueAxis *bpYAxis = new QValueAxis();
    bpYAxis->setRange(60, 180);
    bpYAxis->setTitleText("血压");
    bpYAxis->setTitleFont(QFont("微软雅黑", 14));
    m_bpChart->addAxis(bpYAxis, Qt::AlignLeft);
    spSeries->attachAxis(bpYAxis);
    dpSeries->attachAxis(bpYAxis);

    m_chartViewBp->setChart(m_bpChart);
    qDebug() << "血压图表创建完成";

    qDebug() << "开始创建体温图表...";
    QLineSeries *tempSeries = new QLineSeries();
    tempSeries->setName("体温(℃)");
    for (const HealthData &data : dataList) {
        tempSeries->append(data.recordTime.toMSecsSinceEpoch(), data.temperature);
    }

    m_tempChart = new QChart();
    m_tempChart->addSeries(tempSeries);
    m_tempChart->setTitle("今日体温趋势图");
    m_tempChart->setTitleFont(QFont("微软雅黑", 16));
    m_tempChart->legend()->setFont(QFont("微软雅黑", 14));

    QDateTimeAxis *tempXAxis = new QDateTimeAxis();
    tempXAxis->setFormat("HH:mm");
    tempXAxis->setTitleText("今日时间");
    tempXAxis->setTitleFont(QFont("微软雅黑", 14));
    tempXAxis->setRange(startTime, endTime);
    m_tempChart->addAxis(tempXAxis, Qt::AlignBottom);
    tempSeries->attachAxis(tempXAxis);

    QValueAxis *tempYAxis = new QValueAxis();
    tempYAxis->setRange(35.0, 39.0);
    tempYAxis->setTitleText("体温");
    tempYAxis->setTitleFont(QFont("微软雅黑", 14));
    m_tempChart->addAxis(tempYAxis, Qt::AlignLeft);
    tempSeries->attachAxis(tempYAxis);

    m_chartViewTemp->setChart(m_tempChart);
    qDebug() << "体温图表创建完成";

    qDebug() << "开始创建血氧图表...";
    QLineSeries *spo2Series = new QLineSeries();
    spo2Series->setName("血氧(%)");
    for (const HealthData &data : dataList) {
        spo2Series->append(data.recordTime.toMSecsSinceEpoch(), data.spO2);
    }

    m_spO2Chart = new QChart();
    m_spO2Chart->addSeries(spo2Series);
    m_spO2Chart->setTitle("今日血氧趋势图");
    m_spO2Chart->setTitleFont(QFont("微软雅黑", 16));
    m_spO2Chart->legend()->setFont(QFont("微软雅黑", 14));

    QDateTimeAxis *spo2XAxis = new QDateTimeAxis();
    spo2XAxis->setFormat("HH:mm");
    spo2XAxis->setTitleText("今日时间");
    spo2XAxis->setTitleFont(QFont("微软雅黑", 14));
    spo2XAxis->setRange(startTime, endTime);
    m_spO2Chart->addAxis(spo2XAxis, Qt::AlignBottom);
    spo2Series->attachAxis(spo2XAxis);

    QValueAxis *spo2YAxis = new QValueAxis();
    spo2YAxis->setRange(90, 100);
    spo2YAxis->setTitleText("血氧");
    spo2YAxis->setTitleFont(QFont("微软雅黑", 14));
    m_spO2Chart->addAxis(spo2YAxis, Qt::AlignLeft);
    spo2Series->attachAxis(spo2YAxis);

    m_chartViewSpO2->setChart(m_spO2Chart);
    qDebug() << "血氧图表创建完成";
    qDebug() << "drawHealthChart函数执行完成";
}

void MainWindow::updateCharts()
{
    drawHealthChart(m_currentElderId);
}

void MainWindow::onSOSReceived(const QString &elderId)
{
    QMessageBox::critical(this, "紧急求助",
                          QString("收到老人 %1 的SOS求助信号！\n已推送至家属端。").arg(elderId),
                          QMessageBox::Ok);

    if (m_httpPush) {
        m_httpPush->pushSOS("13800138000", "张大爷", elderId);
    }
}

void MainWindow::onElderChanged(const QString &elderId)
{
    m_currentElderId = elderId;
    drawHealthChart(elderId);
    updateStatisticsDisplay();
}

void MainWindow::onExportData()
{
    if (!m_healthMgr) {
        QMessageBox::warning(this, "错误", "数据库管理器未初始化");
        return;
    }
    
    QString filePath = QFileDialog::getSaveFileName(this, "导出健康数据", "", "CSV文件 (*.csv)");
    if (filePath.isEmpty()) {
        return;
    }

    QDateTime todayStart = QDateTime::currentDateTime().date().startOfDay();
    bool ok = m_healthMgr->exportToCSV(m_currentElderId, todayStart, QDateTime::currentDateTime(), filePath);
    
    if (ok) {
        QMessageBox::information(this, "导出成功", "数据已成功导出到：" + filePath);
    } else {
        QMessageBox::warning(this, "导出失败", "没有可导出的数据或文件写入失败");
    }
}

void MainWindow::onViewStatistics()
{
    if (!m_healthMgr) {
        QMessageBox::warning(this, "错误", "数据库管理器未初始化");
        return;
    }
    
    QDateTime weekStart = QDateTime::currentDateTime().date().addDays(-7).startOfDay();
    HealthDataManager::HealthStatistics stats = 
        m_healthMgr->getStatistics(m_currentElderId, weekStart, QDateTime::currentDateTime());

    QString statsText = QString(
        "老人ID：%1\n"
        "统计周期：最近7天\n"
        "总记录数：%2\n"
        "异常记录数：%3\n\n"
        "心率统计：\n"
        "  平均值：%4 次/分\n"
        "  最大值：%5 次/分\n"
        "  最小值：%6 次/分\n\n"
        "血压统计：\n"
        "  平均收缩压：%7 mmHg\n"
        "  平均舒张压：%8 mmHg\n\n"
        "其他指标：\n"
        "  平均体温：%9 ℃\n"
        "  平均血氧：%10 %"
    ).arg(m_currentElderId)
     .arg(stats.totalRecords)
     .arg(stats.abnormalCount)
     .arg(stats.avgHeartRate, 0, 'f', 1)
     .arg(stats.maxHeartRate, 0, 'f', 1)
     .arg(stats.minHeartRate, 0, 'f', 1)
     .arg(stats.avgSystolic, 0, 'f', 1)
     .arg(stats.avgDiastolic, 0, 'f', 1)
     .arg(stats.avgTemperature, 0, 'f', 1)
     .arg(stats.avgSpO2, 0, 'f', 1);

    QMessageBox::information(this, "健康数据统计", statsText);
}

void MainWindow::onCleanOldData()
{
    if (!m_healthMgr) {
        QMessageBox::warning(this, "错误", "数据库管理器未初始化");
        return;
    }
    
    int ret = QMessageBox::question(this, "确认清理", "确定要清理30天前的旧数据吗？此操作不可恢复。");
    if (ret == QMessageBox::Yes) {
        bool ok = m_healthMgr->cleanOldData(30);
        if (ok) {
            QMessageBox::information(this, "清理成功", "旧数据已成功清理");
        } else {
            QMessageBox::warning(this, "清理失败", "数据清理失败");
        }
    }
}

void MainWindow::onToggleTestData()
{
    if (!m_testDataTimer) {
        qWarning() << "定时器未初始化";
        return;
    }
    
    m_isTestDataRunning = !m_isTestDataRunning;
    if (m_isTestDataRunning) {
        m_testDataTimer->start(3000);
        if (m_btnToggleTest) {
            m_btnToggleTest->setText("停止模拟数据");
        }
    } else {
        m_testDataTimer->stop();
        if (m_btnToggleTest) {
            m_btnToggleTest->setText("启动模拟数据");
        }
    }
}

void MainWindow::onAddElder()
{
    QDialog dialog(this);
    dialog.setWindowTitle("添加老人");
    dialog.setMinimumWidth(400);
    
    QFormLayout *layout = new QFormLayout(&dialog);
    
    QLineEdit *idEdit = new QLineEdit(&dialog);
    idEdit->setPlaceholderText("例如：LAO001");
    layout->addRow("老人ID：", idEdit);
    
    QLineEdit *nameEdit = new QLineEdit(&dialog);
    nameEdit->setPlaceholderText("请输入姓名");
    layout->addRow("姓名：", nameEdit);
    
    QSpinBox *ageSpin = new QSpinBox(&dialog);
    ageSpin->setRange(50, 120);
    ageSpin->setValue(70);
    layout->addRow("年龄：", ageSpin);
    
    QComboBox *genderCombo = new QComboBox(&dialog);
    genderCombo->addItems({"男", "女"});
    layout->addRow("性别：", genderCombo);
    
    QLineEdit *phoneEdit = new QLineEdit(&dialog);
    phoneEdit->setPlaceholderText("请输入联系电话");
    layout->addRow("联系电话：", phoneEdit);
    
    QLineEdit *emergencyContactEdit = new QLineEdit(&dialog);
    emergencyContactEdit->setPlaceholderText("请输入紧急联系人姓名");
    layout->addRow("紧急联系人：", emergencyContactEdit);
    
    QLineEdit *emergencyPhoneEdit = new QLineEdit(&dialog);
    emergencyPhoneEdit->setPlaceholderText("请输入紧急联系人电话");
    layout->addRow("紧急联系人电话：", emergencyPhoneEdit);
    
    QLineEdit *addressEdit = new QLineEdit(&dialog);
    addressEdit->setPlaceholderText("请输入地址");
    layout->addRow("地址：", addressEdit);
    
    QTextEdit *notesEdit = new QTextEdit(&dialog);
    notesEdit->setMaximumHeight(80);
    notesEdit->setPlaceholderText("备注信息（选填）");
    layout->addRow("备注：", notesEdit);
    
    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *okBtn = new QPushButton("确定", &dialog);
    QPushButton *cancelBtn = new QPushButton("取消", &dialog);
    btnLayout->addStretch();
    btnLayout->addWidget(okBtn);
    btnLayout->addWidget(cancelBtn);
    layout->addRow(btnLayout);
    
    connect(okBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
    
    if (dialog.exec() == QDialog::Accepted) {
        QString elderId = idEdit->text().trimmed();
        QString name = nameEdit->text().trimmed();
        
        if (elderId.isEmpty() || name.isEmpty()) {
            QMessageBox::warning(this, "输入错误", "老人ID和姓名不能为空！");
            return;
        }
        
        ElderInfo info;
        info.elderId = elderId;
        info.name = name;
        info.age = ageSpin->value();
        info.gender = genderCombo->currentText();
        info.phone = phoneEdit->text().trimmed();
        info.emergencyContact = emergencyContactEdit->text().trimmed();
        info.emergencyPhone = emergencyPhoneEdit->text().trimmed();
        info.address = addressEdit->text().trimmed();
        info.notes = notesEdit->toPlainText().trimmed();
        
        if (m_healthMgr->addElder(info)) {
            QMessageBox::information(this, "添加成功", QString("老人 %1（%2）已成功添加！").arg(name).arg(elderId));
            
            m_comboElder->clear();
            QStringList elders = m_healthMgr->getElderList();
            m_comboElder->addItems(elders);
            
            int index = m_comboElder->findText(elderId);
            if (index >= 0) {
                m_comboElder->setCurrentIndex(index);
            }
        } else {
            QMessageBox::warning(this, "添加失败", "老人ID可能已存在，或数据库操作失败！");
        }
    }
}

void MainWindow::onManageElders()
{
    if (!m_healthMgr) {
        QMessageBox::warning(this, "错误", "数据库管理器未初始化");
        return;
    }
    
    QDialog dialog(this);
    dialog.setWindowTitle("老人管理");
    dialog.setMinimumSize(800, 500);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);
    
    QTableWidget *table = new QTableWidget(&dialog);
    table->setColumnCount(8);
    table->setHorizontalHeaderLabels({"老人ID", "姓名", "年龄", "性别", "联系电话", "紧急联系人", "紧急电话", "地址"});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    mainLayout->addWidget(table);
    
    QVector<ElderInfo> elders = m_healthMgr->getAllElders();
    table->setRowCount(elders.size());
    
    for (int i = 0; i < elders.size(); i++) {
        const ElderInfo &info = elders[i];
        table->setItem(i, 0, new QTableWidgetItem(info.elderId));
        table->setItem(i, 1, new QTableWidgetItem(info.name));
        table->setItem(i, 2, new QTableWidgetItem(QString::number(info.age)));
        table->setItem(i, 3, new QTableWidgetItem(info.gender));
        table->setItem(i, 4, new QTableWidgetItem(info.phone));
        table->setItem(i, 5, new QTableWidgetItem(info.emergencyContact));
        table->setItem(i, 6, new QTableWidgetItem(info.emergencyPhone));
        table->setItem(i, 7, new QTableWidgetItem(info.address));
    }
    
    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *deleteBtn = new QPushButton("删除选中", &dialog);
    QPushButton *closeBtn = new QPushButton("关闭", &dialog);
    btnLayout->addStretch();
    btnLayout->addWidget(deleteBtn);
    btnLayout->addWidget(closeBtn);
    mainLayout->addLayout(btnLayout);
    
    connect(closeBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(deleteBtn, &QPushButton::clicked, [&]() {
        int row = table->currentRow();
        if (row < 0) {
            QMessageBox::warning(&dialog, "提示", "请先选择要删除的老人！");
            return;
        }
        
        QString elderId = table->item(row, 0)->text();
        QString name = table->item(row, 1)->text();
        
        int ret = QMessageBox::question(&dialog, "确认删除", 
            QString("确定要删除老人 %1（%2）吗？\n注意：只会删除老人信息，健康数据会保留。").arg(name).arg(elderId));
        
        if (ret == QMessageBox::Yes) {
            if (m_healthMgr->removeElder(elderId)) {
                QMessageBox::information(&dialog, "删除成功", QString("老人 %1 已删除！").arg(name));
                table->removeRow(row);
                
                m_comboElder->clear();
                QStringList elders = m_healthMgr->getElderList();
                m_comboElder->addItems(elders);
            } else {
                QMessageBox::warning(&dialog, "删除失败", "删除老人信息失败！");
            }
        }
    });
    
    dialog.exec();
}

void MainWindow::onChartDoubleClicked(int chartIndex)
{
    if (!m_healthMgr) {
        QMessageBox::warning(this, "错误", "数据库管理器未初始化");
        return;
    }
    
    QString title;
    QChart *sourceChart = nullptr;
    
    switch (chartIndex) {
    case 0:
        title = "心率趋势图 - " + m_currentElderId;
        sourceChart = m_hrChart;
        break;
    case 1:
        title = "血压趋势图 - " + m_currentElderId;
        sourceChart = m_bpChart;
        break;
    case 2:
        title = "体温趋势图 - " + m_currentElderId;
        sourceChart = m_tempChart;
        break;
    case 3:
        title = "血氧趋势图 - " + m_currentElderId;
        sourceChart = m_spO2Chart;
        break;
    default:
        return;
    }
    
    if (!sourceChart) {
        QMessageBox::information(this, "提示", "图表尚未生成，请稍后再试或等待数据加载。");
        return;
    }
    
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle(title);
    dialog->setMinimumSize(1000, 700);
    
    QVBoxLayout *layout = new QVBoxLayout(dialog);
    
    QChartView *chartView = new QChartView(dialog);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setStyleSheet(
        "QChartView {"
        "    border: 2px solid #2196F3;"
        "    border-radius: 8px;"
        "    background-color: white;"
        "}"
    );
    
    QChart *newChart = new QChart();
    
    newChart->legend()->setFont(sourceChart->legend()->font());
    newChart->legend()->setAlignment(sourceChart->legend()->alignment());
    
    newChart->setTitle(sourceChart->title());
    newChart->setTitleFont(QFont("微软雅黑", 20));
    newChart->legend()->setFont(QFont("微软雅黑", 16));
    
    for (auto *series : sourceChart->series()) {
        QAbstractSeries *newSeries = nullptr;
        
        if (series->type() == QAbstractSeries::SeriesTypeLine) {
            QLineSeries *lineSeries = qobject_cast<QLineSeries*>(series);
            if (lineSeries) {
                QLineSeries *newLineSeries = new QLineSeries(newChart);
                newLineSeries->setName(lineSeries->name());
                newLineSeries->setPen(lineSeries->pen());
                newLineSeries->append(lineSeries->points());
                newSeries = newLineSeries;
            }
        }
        
        if (newSeries) {
            newChart->addSeries(newSeries);
        }
    }
    
    for (auto *axis : sourceChart->axes()) {
        QDateTimeAxis *dtAxis = qobject_cast<QDateTimeAxis*>(axis);
        QValueAxis *valAxis = qobject_cast<QValueAxis*>(axis);
        
        if (dtAxis) {
            QDateTimeAxis *newAxis = new QDateTimeAxis(newChart);
            newAxis->setFormat(dtAxis->format());
            newAxis->setTitleText(dtAxis->titleText());
            newAxis->setTitleFont(QFont("微软雅黑", 16));
            newAxis->setRange(dtAxis->min(), dtAxis->max());
            newChart->addAxis(newAxis, axis->alignment());
            
            for (auto *series : newChart->series()) {
                if (series->type() == QAbstractSeries::SeriesTypeLine) {
                    series->attachAxis(newAxis);
                }
            }
        } else if (valAxis) {
            QValueAxis *newAxis = new QValueAxis(newChart);
            newAxis->setRange(valAxis->min(), valAxis->max());
            newAxis->setTitleText(valAxis->titleText());
            newAxis->setTitleFont(QFont("微软雅黑", 16));
            newAxis->setTickCount(valAxis->tickCount());
            newChart->addAxis(newAxis, axis->alignment());
            
            for (auto *series : newChart->series()) {
                if (series->type() == QAbstractSeries::SeriesTypeLine) {
                    series->attachAxis(newAxis);
                }
            }
        }
    }
    
    chartView->setChart(newChart);
    layout->addWidget(chartView);
    
    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *closeBtn = new QPushButton("关闭", dialog);
    closeBtn->setStyleSheet(
        "QPushButton {"
        "    background-color: #2196F3;"
        "    color: white;"
        "    border: none;"
        "    padding: 10px 30px;"
        "    border-radius: 5px;"
        "    font-size: 16px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: #1976D2;"
        "}"
    );
    btnLayout->addStretch();
    btnLayout->addWidget(closeBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);
    
    connect(closeBtn, &QPushButton::clicked, dialog, &QDialog::accept);
    
    dialog->exec();
    dialog->deleteLater();
}
