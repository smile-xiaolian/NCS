#pragma once

#include <QWidget>
#include <QtCharts/QChartView>

class QLabel;

class RevenuePage : public QWidget
{
public:
    explicit RevenuePage(QWidget *parent = nullptr);
    void refresh();

private:
    void updateMetrics();
    void rebuildCharts();

    QLabel *mOrderLabel = nullptr;
    QLabel *mRevenueLabel = nullptr;
    QLabel *mOnlineLabel = nullptr;
    QLabel *mUserLabel = nullptr;
    QChartView *mLineView = nullptr;
    QChartView *mBarView = nullptr;
};