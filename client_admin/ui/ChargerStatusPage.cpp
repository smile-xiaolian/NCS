#include "ChargerStatusPage.h"

#include <QBrush>
#include <QColor>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPainter>
#include <QPen>
#include <QSplitter>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>

#include "core/service/PlatformService.h"
#include "FormatUtil.h"
#include "MetricCard.h"


namespace {
void setupTable(QTableWidget *table, const QStringList &headers)
{
    table->setColumnCount(headers.size());
    table->setHorizontalHeaderLabels(headers);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
}
} // namespace

ChargerStatusPage::ChargerStatusPage(QWidget *parent)
    : QWidget(parent)
{
    auto *root = new QVBoxLayout(this);
    auto *title = new QLabel(QStringLiteral("<h2>电桩状态总览</h2>"));
    root->addWidget(title);

    auto *cards = new QHBoxLayout;
    cards->addWidget(ncs::metricCard(QStringLiteral("电桩总数（台）"), &mTotalLabel));
    cards->addWidget(ncs::metricCard(QStringLiteral("空闲"), &mIdleLabel,
                                     QStringLiteral("#34c759")));
    cards->addWidget(ncs::metricCard(QStringLiteral("使用中"), &mBusyLabel,
                                     QStringLiteral("#ff9500")));
    cards->addWidget(ncs::metricCard(QStringLiteral("故障"), &mFaultLabel,
                                     QStringLiteral("#ff3b30")));
    cards->addWidget(ncs::metricCard(QStringLiteral("整体健康度（%）"), &mHealthLabel,
                                     QStringLiteral("#34c759")));
    root->addLayout(cards);

    auto *splitter = new QSplitter(Qt::Horizontal);
    auto *pieBox = new QWidget;
    auto *pieLayout = new QVBoxLayout(pieBox);
    auto *pieCaption = new QLabel(QStringLiteral("充电桩状态分布（饼图）"));
    pieCaption->setStyleSheet(QStringLiteral("font-weight:bold;"));
    mPieView = new QChartView;
    mPieView->setRenderHint(QPainter::Antialiasing);
    pieLayout->addWidget(pieCaption);
    pieLayout->addWidget(mPieView, 1);
    splitter->addWidget(pieBox);

    auto *tableBox = new QWidget;
    auto *tableLayout = new QVBoxLayout(tableBox);
    auto *tableCaption = new QLabel(QStringLiteral("各充电站电桩健康度分布"));
    tableCaption->setStyleSheet(QStringLiteral("font-weight:bold;"));
    mTable = new QTableWidget;
    setupTable(mTable, {QStringLiteral("充电站"), QStringLiteral("桩总数"),
                        QStringLiteral("空闲"), QStringLiteral("使用中"),
                        QStringLiteral("故障"), QStringLiteral("健康度（%）")});
    tableLayout->addWidget(tableCaption);
    tableLayout->addWidget(mTable, 1);
    splitter->addWidget(tableBox);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    root->addWidget(splitter, 1);
}

void ChargerStatusPage::refresh()
{
    const QVariantMap ov = PlatformService::chargerOverview();
    mTotalLabel->setText(QString::number(ov.value(QStringLiteral("total")).toInt()));
    mIdleLabel->setText(QString::number(ov.value(QStringLiteral("idle")).toInt()));
    mBusyLabel->setText(QString::number(ov.value(QStringLiteral("busy")).toInt()));
    mFaultLabel->setText(QString::number(ov.value(QStringLiteral("fault")).toInt()));
    mHealthLabel->setText(ncs::number(ov.value(QStringLiteral("health")).toDouble(), 1));
    rebuildPie();
    fillStationTable();
}

void ChargerStatusPage::rebuildPie()
{
    const QVariantMap ov = PlatformService::chargerOverview();
    const int total = ov.value(QStringLiteral("total")).toInt();

    auto *pie = new QPieSeries;
    const struct {
        QString name;
        QString key;
        QColor color;
    } segments[] = {
        {QStringLiteral("空闲"), QStringLiteral("idle"), QColor(QStringLiteral("#34c759"))},
        {QStringLiteral("使用中"), QStringLiteral("busy"), QColor(QStringLiteral("#ff9500"))},
        {QStringLiteral("故障"), QStringLiteral("fault"), QColor(QStringLiteral("#ff3b30"))},
    };
    for (const auto &segment : segments) {
        const int value = ov.value(segment.key).toInt();
        if (value <= 0) {
            continue;
        }
        QPieSlice *slice = pie->append(segment.name + QStringLiteral("  ") +
                                           QString::number(value),
                                       value);
        slice->setBrush(segment.color);
        slice->setPen(QPen(QColor(QStringLiteral("#ffffff"))));
    }

    auto *chart = new QChart;
    chart->addSeries(pie);
    chart->setTitle(total > 0
                        ? QStringLiteral("电桩状态分布（共 %1 台）").arg(total)
                        : QStringLiteral("电桩状态分布"));
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);
    mPieView->setChart(chart);
}

void ChargerStatusPage::fillStationTable()
{
    const QVariantList stations = PlatformService::stations();
    mTable->setRowCount(stations.size());

    int row = 0;
    for (const QVariant &item : stations) {
        const QVariantMap station = item.toMap();
        const int stationId = station.value(QStringLiteral("id")).toInt();
        const QVariantList chargers = PlatformService::chargers(stationId);

        int idle = 0;
        int busy = 0;
        int fault = 0;
        for (const QVariant &chargerItem : chargers) {
            const QVariantMap charger = chargerItem.toMap();
            switch (charger.value(QStringLiteral("status")).toInt()) {
            case 0: ++idle; break;
            case 1: ++busy; break;
            case 2: ++fault; break;
            default: break;
            }
        }
        const int total = chargers.size();
        const double health = total > 0 ? 100.0 * (total - fault) / total : 0.0;

        QTableWidgetItem *first = new QTableWidgetItem(station.value(QStringLiteral("name")).toString());
        first->setData(Qt::UserRole, stationId);
        mTable->setItem(row, 0, first);
        mTable->setItem(row, 1, new QTableWidgetItem(QString::number(total)));
        mTable->setItem(row, 2, new QTableWidgetItem(QString::number(idle)));
        mTable->setItem(row, 3, new QTableWidgetItem(QString::number(busy)));
        mTable->setItem(row, 4, new QTableWidgetItem(QString::number(fault)));
        mTable->setItem(row, 5, new QTableWidgetItem(ncs::number(health, 1)));
        ++row;
    }
}