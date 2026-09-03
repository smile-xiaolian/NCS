#pragma once

#include <QWidget>
#include <QtCharts/QChartView>

class QLabel;
class QTableWidget;

class ChargerStatusPage : public QWidget
{
public:
    explicit ChargerStatusPage(QWidget *parent = nullptr);
    void refresh();

private:
    void rebuildPie();
    void fillStationTable();

    QLabel *mTotalLabel = nullptr;
    QLabel *mIdleLabel = nullptr;
    QLabel *mBusyLabel = nullptr;
    QLabel *mFaultLabel = nullptr;
    QLabel *mHealthLabel = nullptr;
    QChartView *mPieView = nullptr;
    QTableWidget *mTable = nullptr;
};