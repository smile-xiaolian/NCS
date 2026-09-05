#include "StationDetailPage.h"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include "core/service/PlatformService.h"

StationDetailPage::StationDetailPage(QWidget *parent) : QWidget(parent) {
    setStyleSheet("background-color: #f3f0ff;"); // 独立柔和紫罗兰配色
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(15, 15, 15, 15);

    stationTitleLabel = new QLabel;
    stationTitleLabel->setStyleSheet("font-size: 15pt; font-weight: bold; color: #5b21b6; margin-bottom: 5px;");
    layout->addWidget(stationTitleLabel);

    auto tip = new QLabel("请选择空闲电桩进行预约（余额需 ≥ 5 元）：");
    tip->setStyleSheet("color: #6b21a8; font-weight: bold;");
    layout->addWidget(tip);

    chargerTable = new QTableWidget;
    chargerTable->setColumnCount(4);
    chargerTable->setHorizontalHeaderLabels({"编号", "类型", "功率", "状态"});
    chargerTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    chargerTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    chargerTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    chargerTable->verticalHeader()->setVisible(false);
    layout->addWidget(chargerTable);

    reserveBtn = new QPushButton("预约选中电桩");
    reserveBtn->setStyleSheet("background-color: #7c3aed; color: white; font-weight: bold; border-radius: 6px; padding: 10px;");
    navigateBtn = new QPushButton("一键导航 (腾讯地图)");
    navigateBtn->setObjectName("secondaryBtn");
    backHomeBtn = new QPushButton("返回首页");
    backHomeBtn->setObjectName("secondaryBtn");

    layout->addWidget(reserveBtn);
    layout->addWidget(navigateBtn);
    layout->addWidget(backHomeBtn);

    connect(backHomeBtn, &QPushButton::clicked, this, &StationDetailPage::requestBackHome);

    connect(navigateBtn, &QPushButton::clicked, this, [this] {
        auto s = PlatformService::station(currentStationId);
        QDesktopServices::openUrl(QUrl(QString("https://apis.map.qq.com/uri/v1/routeplan?type=drive&to=%1&coord=%2,%3")
            .arg(s["name"].toString()).arg(s["latitude"].toDouble()).arg(s["longitude"].toDouble())));
    });

    connect(reserveBtn, &QPushButton::clicked, this, [this] {
        auto item = chargerTable->currentItem();
        if (!item) { QMessageBox::information(this, "提示", "请选择电桩"); return; }
        int chargerId = chargerTable->item(item->row(), 0)->data(Qt::UserRole).toInt();
        // 外部由主窗口或全局提供 userId，这里通过信号槽或外部调用完成预约
        emit reserveSuccess(); // 触发主窗口处理预约业务
    });
}

void StationDetailPage::loadStation(int stationId) {
    currentStationId = stationId;
    auto sInfo = PlatformService::station(stationId);
    stationTitleLabel->setText("🔌 " + sInfo["name"].toString() + " - 电桩列表");

    auto a = PlatformService::chargers(stationId);
    chargerTable->setRowCount(a.size());
    for (int i = 0; i < a.size(); i++) {
        auto z = a[i].toMap();
        int stStatus = z["status"].toInt();
        QString statusStr = stStatus == 0 ? "空闲" : (stStatus == 1 ? "使用中" : "故障");
        QStringList v = { z["code"].toString(), z["type"].toString(), QString::number(z["power"].toDouble()) + " kW", statusStr };
        for (int j = 0; j < 4; j++) {
            auto item = new QTableWidgetItem(v[j]);
            if (j == 3) item->setForeground(stStatus == 0 ? QColor("#52c41a") : (stStatus == 1 ? QColor("#fa8c16" ) : QColor("#ff4d4f")));
            chargerTable->setItem(i, j, item);
        }
        chargerTable->item(i, 0)->setData(Qt::UserRole, z["id"]);
    }
}

int getSelectedChargerId(QTableWidget *table) {
    auto item = table->currentItem();
    if (!item) return 0;
    return table->item(item->row(), 0)->data(Qt::UserRole).toInt();
}