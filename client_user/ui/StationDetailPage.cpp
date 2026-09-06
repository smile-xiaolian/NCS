#include "StationDetailPage.h"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include "core/service/PlatformService.h"

StationDetailPage::StationDetailPage(QWidget *parent) : QWidget(parent)
{
    setStyleSheet("background-color: #f3f0ff;");

    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(15, 15, 15, 15);

    titleLabel = new QLabel;
    titleLabel->setStyleSheet("font-size: 15pt; font-weight: bold; color: #5b21b6; margin-bottom: 5px;");
    layout->addWidget(titleLabel);

    auto detailTip = new QLabel("请选择空闲电桩进行预约（余额需 ≥ 5 元）：");
    detailTip->setStyleSheet("color: #6b21a8; font-weight: bold;");
    layout->addWidget(detailTip);

    chargerTable = new QTableWidget;
    setupTable();
    layout->addWidget(chargerTable);

    reserveBtn = new QPushButton("预约选中电桩");
    reserveBtn->setStyleSheet("background-color: #7c3aed; color: white; font-weight: bold; border-radius: 6px; padding: 10px;");

    navigateBtn = new QPushButton("一键导航");
    navigateBtn->setObjectName("secondaryBtn");

    backBtn = new QPushButton("返回首页");
    backBtn->setObjectName("secondaryBtn");

    layout->addWidget(reserveBtn);
    layout->addWidget(navigateBtn);
    layout->addWidget(backBtn);

    connect(reserveBtn, &QPushButton::clicked, this, &StationDetailPage::onReserve);
    connect(navigateBtn, &QPushButton::clicked, this, &StationDetailPage::onNavigate);
    connect(backBtn, &QPushButton::clicked, this, &StationDetailPage::backToHomeRequested);
}

void StationDetailPage::setupTable()
{
    QStringList headers = { "编号", "类型", "功率", "状态" };
    chargerTable->setColumnCount(headers.size());
    chargerTable->setHorizontalHeaderLabels(headers);
    chargerTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    chargerTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    chargerTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    chargerTable->verticalHeader()->setVisible(false);
}

void StationDetailPage::loadStation(int stationId, int userId)
{
    currentStationId = stationId;
    currentUserId = userId;

    auto sInfo = PlatformService::station(stationId);
    titleLabel->setText(sInfo["name"].toString() + " - 电桩列表");

    auto list = PlatformService::chargers(stationId);
    chargerTable->setRowCount(list.size());
    for (int i = 0; i < list.size(); i++)
    {
        auto z = list[i].toMap();
        int stStatus = z["status"].toInt();
        QString statusStr = stStatus == 0 ? "空闲" : (stStatus == 1 ? "使用中" : "故障");
        QStringList v = { z["code"].toString(), z["type"].toString(), QString::number(z["power"].toDouble()) + " kW", statusStr };

        for (int j = 0; j < 4; j++)
        {
            auto item = new QTableWidgetItem(v[j]);
            if (j == 3) {
                item->setForeground(stStatus == 0 ? QColor("#52c41a") : (stStatus == 1 ? QColor("#fa8c16") : QColor("#ff4d4f")));
            }
            chargerTable->setItem(i, j, item);
        }
        chargerTable->item(i, 0)->setData(Qt::UserRole, z["id"]);
    }
}

void StationDetailPage::onReserve()
{
    auto x = chargerTable->currentItem();
    if (!x) {
        QMessageBox::information(this, "提示", "请选择电桩");
        return;
    }

    QString errorMsg;
    int chargerId = chargerTable->item(x->row(), 0)->data(Qt::UserRole).toInt();
    if (PlatformService::reserve(currentUserId, chargerId, &errorMsg))
    {
        QMessageBox::information(this, "提示", "预约成功！即将进入充电控制与结算页");
        emit reservationSuccess();
    }
    else {
        QMessageBox::warning(this, "预约失败", errorMsg);
    }
}

void StationDetailPage::onNavigate()
{
    if (!currentStationId) return;
    auto s = PlatformService::station(currentStationId);
    emit navigateRequested(s["latitude"].toDouble(), s["longitude"].toDouble(), s["name"].toString());
}