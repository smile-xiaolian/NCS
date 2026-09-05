#include "HomePage.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include "core/service/PlatformService.h"

HomePage::HomePage(QWidget *parent) : QWidget(parent) {
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(15, 15, 15, 15);
    layout->setSpacing(10);

    auto locateLayout = new QHBoxLayout();
    regionCombo = new QComboBox;
    regionCombo->addItem("上海·人民广场", QVariant::fromValue(QPointF(31.2304, 121.4737)));
    regionCombo->addItem("上海·陆家嘴", QVariant::fromValue(QPointF(31.2393, 121.5000)));
    addressEdit = new QLineEdit;
    addressEdit->setPlaceholderText("搜索区域或详细地址");
    locateBtn = new QPushButton("定位");

    locateLayout->addWidget(regionCombo, 2);
    locateLayout->addWidget(addressEdit, 2);
    locateLayout->addWidget(locateBtn, 1);
    layout->addLayout(locateLayout);

    layout->addWidget(new QLabel("<b>附近优质充电站（点击卡片或双击直接预约）</b>"));

    stationTable = new QTableWidget;
    stationTable->setColumnCount(4);
    stationTable->setHorizontalHeaderLabels({"站点名称", "单价", "空闲/总数", "距离"});
    stationTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    stationTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    stationTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    stationTable->verticalHeader()->setVisible(false);
    layout->addWidget(stationTable);

    enterDetailBtn = new QPushButton("进入选桩详情页");
    layout->addWidget(enterDetailBtn);

    connect(locateBtn, &QPushButton::clicked, this, [this] {
        QPointF coords = regionCombo->currentData().toPointF();
        currentLat = coords.x();
        currentLng = coords.y();
        refreshStations();
    });

    auto handleSelect = [this] {
        auto item = stationTable->currentItem();
        if (!item) return;
        int stationId = stationTable->item(item->row(), 0)->data(Qt::UserRole).toInt();
        emit requestStationDetail(stationId);
    };

    connect(stationTable, &QTableWidget::doubleClicked, this, handleSelect);
    connect(enterDetailBtn, &QPushButton::clicked, this, handleSelect);
}

void HomePage::refreshStations() {
    auto a = PlatformService::stations(currentLat, currentLng);
    stationTable->setRowCount(a.size());

    for (int i = 0; i < a.size(); i++) {
        auto m = a[i].toMap();
        QStringList v = {
            m["name"].toString(),
            QString("%1 元/度").arg(m["price"].toDouble(), 0, 'f', 2),
            QString("%1 / %2 桩").arg(m["idle"].toInt()).arg(m["total"].toInt()),
            QString("%1 km").arg(m["distance"].toDouble(), 0, 'f', 1)
        };
        for (int j = 0; j < v.size(); j++) {
            auto item = new QTableWidgetItem(v[j]);
            if (j == 2) item->setForeground(m["idle"].toInt() > 0 ? QColor("#52c41a") : QColor("#ff4d4f"));
            stationTable->setItem(i, j, item);
        }
        stationTable->item(i, 0)->setData(Qt::UserRole, m["id"]);
    }
}