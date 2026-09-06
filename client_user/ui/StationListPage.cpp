#include "StationListPage.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QMessageBox>
#include <QPointF>
#include "core/service/PlatformService.h"

StationListPage::StationListPage(QWidget *parent) : QWidget(parent)
{
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(15, 15, 15, 15);
    layout->setSpacing(10);

    auto locateLayout = new QHBoxLayout();
    regionCombo = new QComboBox;
    regionCombo->addItem("上海·人民广场", QVariant::fromValue(QPointF(31.2304, 121.4737)));
    regionCombo->addItem("上海·陆家嘴", QVariant::fromValue(QPointF(31.2393, 121.5000)));
    regionCombo->addItem("北京·天安门", QVariant::fromValue(QPointF(39.9042, 116.4074)));
    regionCombo->addItem("深圳·福田", QVariant::fromValue(QPointF(22.5431, 114.0579)));

    addressEdit = new QLineEdit;
    addressEdit->setPlaceholderText("搜索区域或详细地址");

    locateBtn = new QPushButton("定位");

    locateLayout->addWidget(regionCombo, 2);
    locateLayout->addWidget(addressEdit, 2);
    locateLayout->addWidget(locateBtn, 1);
    layout->addLayout(locateLayout);

    layout->addWidget(new QLabel("<b>附近优质充电站（点击卡片直接预约）</b>"));

    stationTable = new QTableWidget;
    setupTable();
    layout->addWidget(stationTable);

    auto enterDetailBtn = new QPushButton("进入选桩详情页");
    layout->addWidget(enterDetailBtn);

    connect(locateBtn, &QPushButton::clicked, this, &StationListPage::onLocate);
    connect(enterDetailBtn, &QPushButton::clicked, this, &StationListPage::onStationClick);
    connect(stationTable, &QTableWidget::doubleClicked, this, &StationListPage::onStationClick);
}

void StationListPage::setupTable()
{
    QStringList headers = { "站点名称", "单价", "空闲/总数", "距离" };
    stationTable->setColumnCount(headers.size());
    stationTable->setHorizontalHeaderLabels(headers);
    stationTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    stationTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    stationTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    stationTable->verticalHeader()->setVisible(false);
    stationTable->setStyleSheet(
        "QTableWidget { background-color: #ffffff; alternate-background-color: #f9f9f9; border: 1px solid #e0e0e0; border-radius: 8px; gridline-color: #f0f0f0; }"
        "QHeaderView::section { background-color: #f5f7fa; padding: 8px; border: none; font-weight: bold; color: #333333; }"
    );
}

void StationListPage::refreshStations()
{
    auto list = PlatformService::stations(currentLat, currentLng);
    stationTable->setRowCount(list.size());

    for (int i = 0; i < list.size(); i++)
    {
        auto m = list[i].toMap();
        QStringList v = {
            m["name"].toString(),
            QString("%1 元/度").arg(m["price"].toDouble(), 0, 'f', 2),
            QString("%1 / %2 桩").arg(m["idle"].toInt()).arg(m["total"].toInt()),
            QString("%1 km").arg(m["distance"].toDouble(), 0, 'f', 1)
        };

        for (int j = 0; j < v.size(); j++)
        {
            auto item = new QTableWidgetItem(v[j]);
            if (j == 2) {
                item->setForeground(m["idle"].toInt() > 0 ? QColor("#52c41a") : QColor("#ff4d4f"));
            }
            stationTable->setItem(i, j, item);
        }
        stationTable->item(i, 0)->setData(Qt::UserRole, m["id"]);
    }
}

void StationListPage::onLocate()
{
    QPointF coords = regionCombo->currentData().toPointF();
    currentLat = coords.x();
    currentLng = coords.y();
    if (!addressEdit->text().trimmed().isEmpty()) {
        QMessageBox::information(this, "提示", QString("已使用“%1”的预置坐标进行测算。").arg(regionCombo->currentText()));
    }
    refreshStations();
}

void StationListPage::onStationClick()
{
    auto item = stationTable->currentItem();
    if (!item) {
        QMessageBox::information(this, "提示", "请先选择一个充电站");
        return;
    }
    int stationId = stationTable->item(item->row(), 0)->data(Qt::UserRole).toInt();
    emit stationSelected(stationId);
}