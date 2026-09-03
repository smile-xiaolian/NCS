#include "RevenuePage.h"

#include <QColor>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QSplitter>
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
#include "MetricCard.h"


RevenuePage::RevenuePage(QWidget *parent)
    : QWidget(parent)
{
    auto *root = new QVBoxLayout(this);
    auto *title = new QLabel(QStringLiteral("<h2>营收分析</h2>"));
    root->addWidget(title);

    auto *cards = new QHBoxLayout;
    cards->addWidget(ncs::metricCard(QStringLiteral("完成订单（单）"), &mOrderLabel));
    cards->addWidget(ncs::metricCard(QStringLiteral("总营收（元）"), &mRevenueLabel));
    cards->addWidget(ncs::metricCard(QStringLiteral("在线电桩（台）"), &mOnlineLabel));
    cards->addWidget(ncs::metricCard(QStringLiteral("注册用户（人）"), &mUserLabel));
    root->addLayout(cards);

    auto *splitter = new QSplitter(Qt::Horizontal);

    auto *lineBox = new QWidget;
    auto *lineLayout = new QVBoxLayout(lineBox);
    auto *lineCaption = new QLabel(QStringLiteral("近 30 日营收趋势（完成订单）"));
    lineCaption->setStyleSheet(QStringLiteral("font-weight:bold;"));
    mLineView = new QChartView;
    mLineView->setRenderHint(QPainter::Antialiasing);
    lineLayout->addWidget(lineCaption);
    lineLayout->addWidget(mLineView, 1);
    splitter->addWidget(lineBox);

    auto *barBox = new QWidget;
    auto *barLayout = new QVBoxLayout(barBox);
    auto *barCaption = new QLabel(QStringLiteral("近 30 日每日订单数"));
    barCaption->setStyleSheet(QStringLiteral("font-weight:bold;"));
    mBarView = new QChartView;
    mBarView->setRenderHint(QPainter::Antialiasing);
    barLayout->addWidget(barCaption);
    barLayout->addWidget(mBarView, 1);
    splitter->addWidget(barBox);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    root->addWidget(splitter, 1);
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
    lineSeries->setColor(QColor(QStringLiteral("#1f6feb")));

    auto *barSet = new QBarSet(QStringLiteral("日订单数"));
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
    lineChart->addSeries(lineSeries);
    lineChart->setTitle(QStringLiteral("近 30 日营收趋势（元）"));
    lineChart->legend()->setVisible(true);
    lineChart->legend()->setAlignment(Qt::AlignBottom);
    auto *lineAxisX = new QBarCategoryAxis;
    lineAxisX->append(days);
    lineChart->addAxis(lineAxisX, Qt::AlignBottom);
    lineSeries->attachAxis(lineAxisX);
    auto *lineAxisY = new QValueAxis;
    lineAxisY->setLabelFormat(QStringLiteral("%.0f"));
    lineChart->addAxis(lineAxisY, Qt::AlignLeft);
    lineSeries->attachAxis(lineAxisY);
    mLineView->setChart(lineChart);

    // 柱状图：近 30 日订单数
    auto *barSeries = new QBarSeries;
    barSeries->append(barSet);
    auto *barChart = new QChart;
    barChart->addSeries(barSeries);
    barChart->setTitle(QStringLiteral("近 30 日每日订单数"));
    barChart->legend()->setVisible(true);
    barChart->legend()->setAlignment(Qt::AlignBottom);
    auto *barAxisX = new QBarCategoryAxis;
    barAxisX->append(days);
    barChart->addAxis(barAxisX, Qt::AlignBottom);
    barSeries->attachAxis(barAxisX);
    auto *barAxisY = new QValueAxis;
    barAxisY->setLabelFormat(QStringLiteral("%.0f"));
    barChart->addAxis(barAxisY, Qt::AlignLeft);
    barSeries->attachAxis(barAxisY);
    mBarView->setChart(barChart);
}