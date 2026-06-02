#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QChartView>
#include <QChart>
#include <QTimer>
#include <QComboBox>
#include <QPushButton>
#include <QStringList>
#include "healthdatamanager.h"
#include "serialdevice.h"
#include "httppushservice.h"

class ClickableChartView : public QChartView
{
    Q_OBJECT
public:
    explicit ClickableChartView(int index, QWidget *parent = nullptr) 
        : QChartView(parent), m_index(index) {}
    
    int chartIndex() const { return m_index; }

signals:
    void doubleClicked(int chartIndex);

protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override {
        QChartView::mouseDoubleClickEvent(event);
        emit doubleClicked(m_index);
    }

private:
    int m_index;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onHealthDataReceived(const QString &elderId, const HealthData &data);
    void drawHealthChart(const QString &elderId);
    void onSOSReceived(const QString &elderId);
    void onElderChanged(const QString &elderId);
    void onExportData();
    void onViewStatistics();
    void onCleanOldData();
    void onToggleTestData();
    void onAddElder();
    void onManageElders();
    void onChartDoubleClicked(int chartIndex);
    void updateCharts();

private:
    void setupUI();
    void setupMenuBar();
    void updateDataDisplay(const HealthData &data);
    void updateStatisticsDisplay();
    
    QLabel *m_labelHr;
    QLabel *m_labelBp;
    QLabel *m_labelTemp;
    QLabel *m_labelSpO2;
    QLabel *m_labelStep;
    QLabel *m_labelEmotion;
    QLabel *m_labelPressure;
    QLabel *m_labelBattery;
    QLabel *m_labelSOS;
    QLabel *m_labelTime;
    QLabel *m_labelStats;

    QComboBox *m_comboElder;
    QPushButton *m_btnToggleTest;
    QPushButton *m_btnExport;
    QPushButton *m_btnStats;
    QPushButton *m_btnClean;
    QPushButton *m_btnAddElder;
    QPushButton *m_btnManageElders;

    ClickableChartView *m_chartViewHr;
    ClickableChartView *m_chartViewBp;
    ClickableChartView *m_chartViewTemp;
    ClickableChartView *m_chartViewSpO2;

    QChart *m_hrChart;
    QChart *m_bpChart;
    QChart *m_tempChart;
    QChart *m_spO2Chart;

    HealthDataManager *m_healthMgr;
    SerialDevice *m_serialDevice;
    HttpPushService *m_httpPush;
    QTimer *m_testDataTimer;
    QTimer *m_chartUpdateTimer;
    
    QString m_currentElderId;
    bool m_isTestDataRunning = true;
};

#endif
