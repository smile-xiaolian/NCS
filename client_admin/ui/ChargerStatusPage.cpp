#include "ChargerStatusPage.h"

#include <QBrush>
#include <QColor>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPainter>
#include <QPen>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>

#include "core/service/PlatformService.h"
#include "FormatUtil.h"
#include "UiKit.h"


namespace {
void setupTable(QTableWidget *table, const QStringList &headers)
{
    table->setColumnCount(headers.size());
    table->setHorizontalHeaderLabels(headers);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->horizontalHeader()->setMinimumHeight(42);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setShowGrid(false);
    table->setAlternatingRowColors(true);
    table->verticalHeader()->setVisible(false);
    table->verticalHeader()->setDefaultSectionSize(40);
}
} // namespace

ChargerStatusPage::ChargerStatusPage(QWidget *parent)
    : QWidget(parent)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(14);
    root->addWidget(ncs::pageHeading(QStringLiteral("电桩状态总览"),
                                     QStringLiteral("全网电桩实时运行与健康度监控")));

    auto *cards = new QHBoxLayout;
    cards->setSpacing(12);
    cards->addWidget(ncs::metricCard(QStringLiteral("电桩总数（台）"), &mTotalLabel));
    cards->addWidget(ncs::metricCard(QStringLiteral("空闲"), &mIdleLabel,
                                     QStringLiteral("green")));
    cards->addWidget(ncs::metricCard(QStringLiteral("使用中"), &mBusyLabel,
                                     QStringLiteral("orange")));
    cards->addWidget(ncs::metricCard(QStringLiteral("故障"), &mFaultLabel,
                                     QStringLiteral("red")));
    cards->addWidget(ncs::metricCard(QStringLiteral("整体健康度（%）"), &mHealthLabel,
                                     QStringLiteral("purple")));
    root->addLayout(cards);

    auto *chartRow = new QHBoxLayout;
    chartRow->setSpacing(16);

    const ncs::Panel piePanel =
        ncs::titledPanel(QStringLiteral("电桩状态分布"), nullptr,
                         QStringLiteral("实时占比"));
    mPieView = new QChartView;
    mPieView->setRenderHint(QPainter::Antialiasing);
    piePanel.body->addWidget(mPieView, 1);
    chartRow->addWidget(piePanel.card, 1);

    const ncs::Panel tablePanel =
        ncs::titledPanel(QStringLiteral("各充电站健康度"), nullptr,
                         QStringLiteral("按电站统计"));
    mTable = new QTableWidget;
    setupTable(mTable, {QStringLiteral("充电站"), QStringLiteral("桩总数"),
                        QStringLiteral("空闲"), QStringLiteral("使用中"),
                        QStringLiteral("故障"), QStringLiteral("健康度（%）")});
    tablePanel.body->addWidget(mTable, 1);
    chartRow->addWidget(tablePanel.card, 1);

    root->addLayout(chartRow, 1);
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

    auto *pie = new QPieSeries;
    const struct {
        QString name;
        QString key;
        QColor color;
    } segments[] = {
        {QStringLiteral("空闲"), QStringLiteral("idle"), QColor(QStringLiteral("#27AE60"))},
        {QStringLiteral("使用中"), QStringLiteral("busy"), QColor(QStringLiteral("#F2994A"))},
        {QStringLiteral("故障"), QStringLiteral("fault"), QColor(QStringLiteral("#EB5757"))},
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
        slice->setLabelVisible(false);
        slice->setPen(QPen(QColor(QStringLiteral("#FFFFFF")), 2));
    }

    auto *chart = new QChart;
    chart->setBackgroundVisible(false);
    chart->addSeries(pie);
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);
    chart->legend()->setLabelColor(QColor(QStringLiteral("#7A8699")));
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

        QTableWidgetItem *idleItem = new QTableWidgetItem(QString::number(idle));
        idleItem->setForeground(QColor(QStringLiteral("#1E9E62")));
        QTableWidgetItem *busyItem = new QTableWidgetItem(QString::number(busy));
        busyItem->setForeground(QColor(QStringLiteral("#E8890C")));
        QTableWidgetItem *faultItem = new QTableWidgetItem(QString::number(fault));
        faultItem->setForeground(fault > 0 ? QColor(QStringLiteral("#D64545"))
                                           : QColor(QStringLiteral("#8A94A6")));
        QTableWidgetItem *healthItem = new QTableWidgetItem(ncs::number(health, 1));
        healthItem->setForeground(health >= 95 ? QColor(QStringLiteral("#1E9E62"))
                                               : QColor(QStringLiteral("#E8890C")));
        mTable->setItem(row, 2, idleItem);
        mTable->setItem(row, 3, busyItem);
        mTable->setItem(row, 4, faultItem);
        mTable->setItem(row, 5, healthItem);
        ++row;
    }
}
