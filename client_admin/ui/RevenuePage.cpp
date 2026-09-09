#include "RevenuePage.h"

#include <QColor>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPen>
#include <QVBoxLayout>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

#include "core/service/PlatformService.h"
#include "FormatUtil.h"
#include "UiKit.h"


RevenuePage::RevenuePage(QWidget *parent)
    : QWidget(parent)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(14);
    root->addWidget(ncs::pageHeading(QStringLiteral("营收分析"),
                                     QStringLiteral("近 30 日经营与订单数据概览")));

    auto *cards = new QHBoxLayout;
    cards->setSpacing(12);
    cards->addWidget(ncs::metricCard(QStringLiteral("完成订单（单）"), &mOrderLabel,
                                     QStringLiteral("blue")));
    cards->addWidget(ncs::metricCard(QStringLiteral("总营收（元）"), &mRevenueLabel,
                                     QStringLiteral("green")));
    cards->addWidget(ncs::metricCard(QStringLiteral("在线电桩（台）"), &mOnlineLabel,
                                     QStringLiteral("cyan")));
    cards->addWidget(ncs::metricCard(QStringLiteral("注册用户（人）"), &mUserLabel,
                                     QStringLiteral("purple")));
    root->addLayout(cards);

    auto *chartRow = new QHBoxLayout;
    chartRow->setSpacing(16);

    const ncs::Panel linePanel =
        ncs::titledPanel(QStringLiteral("近 30 日营收趋势"), nullptr,
                         QStringLiteral("单位：元"));
    mLineView = new QChartView;
    mLineView->setRenderHint(QPainter::Antialiasing);
    linePanel.body->addWidget(mLineView, 1);
    chartRow->addWidget(linePanel.card, 1);

    const ncs::Panel barPanel =
        ncs::titledPanel(QStringLiteral("近 30 日每日订单数"), nullptr,
                         QStringLiteral("单位：单"));
    mBarView = new QChartView;
    mBarView->setRenderHint(QPainter::Antialiasing);
    barPanel.body->addWidget(mBarView, 1);
    chartRow->addWidget(barPanel.card, 1);

    root->addLayout(chartRow, 1);
}

void RevenuePage::refresh()
{
    updateMetrics();
    rebuildCharts();
}

void RevenuePage::updateMetrics()
{
    const QVariantMap m = PlatformService::metrics();
    mOrderLabel->setText(QString::number(m.value(QStringLiteral("orders")).toInt()));
    mRevenueLabel->setText(ncs::number(m.value(QStringLiteral("revenue")).toDouble()));
    mOnlineLabel->setText(QString::number(m.value(QStringLiteral("online")).toInt()));
    mUserLabel->setText(QString::number(m.value(QStringLiteral("users")).toInt()));
}

void RevenuePage::rebuildCharts()
{
    const QVariantList daysData = PlatformService::revenueDays(30);

    QStringList days;
    auto *lineSeries = new QLineSeries;
    lineSeries->setName(QStringLiteral("日营收（元）"));
    lineSeries->setColor(QColor(QStringLiteral("#4C8DFF")));
    lineSeries->setPointsVisible(true);
    QPen linePen(QColor(QStringLiteral("#4C8DFF")));
    linePen.setWidth(2);
    lineSeries->setPen(linePen);

    auto *barSet = new QBarSet(QStringLiteral("日订单数"));
    barSet->setColor(QColor(QStringLiteral("#4C8DFF")));
    int index = 0;
    for (const QVariant &item : daysData) {
        const QVariantMap row = item.toMap();
        days << row.value(QStringLiteral("day")).toString();
        lineSeries->append(index, row.value(QStringLiteral("revenue")).toDouble());
        *barSet << row.value(QStringLiteral("orders")).toInt();
        ++index;
    }

    // 折线图：近 30 日营收
    auto *lineChart = new QChart;
    lineChart->setBackgroundVisible(false);
    lineChart->addSeries(lineSeries);
    lineChart->legend()->setVisible(true);
    lineChart->legend()->setAlignment(Qt::AlignBottom);
    lineChart->legend()->setLabelColor(QColor(QStringLiteral("#7A8699")));
    auto *lineAxisX = new QBarCategoryAxis;
    lineAxisX->append(days);
    lineAxisX->setGridLineVisible(false);
    lineAxisX->setLabelsColor(QColor(QStringLiteral("#8A94A6")));
    lineChart->addAxis(lineAxisX, Qt::AlignBottom);
    lineSeries->attachAxis(lineAxisX);
    auto *lineAxisY = new QValueAxis;
    lineAxisY->setLabelFormat(QStringLiteral("%.0f"));
    lineAxisY->setGridLineColor(QColor(QStringLiteral("#E9EEF5")));
    lineAxisY->setLabelsColor(QColor(QStringLiteral("#8A94A6")));
    lineChart->addAxis(lineAxisY, Qt::AlignLeft);
    lineSeries->attachAxis(lineAxisY);
    mLineView->setChart(lineChart);

    // 柱状图：近 30 日订单数
    auto *barSeries = new QBarSeries;
    barSeries->append(barSet);
    auto *barChart = new QChart;
    barChart->setBackgroundVisible(false);
    barChart->addSeries(barSeries);
    barChart->legend()->setVisible(true);
    barChart->legend()->setAlignment(Qt::AlignBottom);
    barChart->legend()->setLabelColor(QColor(QStringLiteral("#7A8699")));
    auto *barAxisX = new QBarCategoryAxis;
    barAxisX->append(days);
    barAxisX->setGridLineVisible(false);
    barAxisX->setLabelsColor(QColor(QStringLiteral("#8A94A6")));
    barChart->addAxis(barAxisX, Qt::AlignBottom);
    barSeries->attachAxis(barAxisX);
    auto *barAxisY = new QValueAxis;
    barAxisY->setLabelFormat(QStringLiteral("%.0f"));
    barAxisY->setGridLineColor(QColor(QStringLiteral("#E9EEF5")));
    barAxisY->setLabelsColor(QColor(QStringLiteral("#8A94A6")));
    barChart->addAxis(barAxisY, Qt::AlignLeft);
    barSeries->attachAxis(barAxisY);
    mBarView->setChart(barChart);
}
