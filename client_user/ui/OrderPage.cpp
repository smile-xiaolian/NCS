#include "OrderPage.h"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QDateTime>
#include "core/service/PlatformService.h"

OrderPage::OrderPage(QWidget *parent) : QWidget(parent) {
    auto outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);

    subStack = new QStackedWidget;
    outerLayout->addWidget(subStack);

    // 1. 历史订单列表页 (Index 0)
    auto listWidget = new QWidget;
    auto listLayout = new QVBoxLayout(listWidget);
    listLayout->setContentsMargins(15, 15, 15, 15);
    listLayout->addWidget(new QLabel("<b>历史与进行中订单（点击进行中订单可去结算）</b>"));
    
    orderTable = new QTableWidget;
    orderTable->setColumnCount(6);
    orderTable->setHorizontalHeaderLabels({"电站", "电桩", "状态", "电量", "金额", "时间"});
    orderTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    orderTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    orderTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    orderTable->verticalHeader()->setVisible(false);
    listLayout->addWidget(orderTable);

    auto viewDetailBtn = new QPushButton("查看/结算选中订单");
    listLayout->addWidget(viewDetailBtn);
    subStack->addWidget(listWidget);

    // 2. 充电控制与结算页 (Index 1)
    auto settleWidget = new QWidget;
    settleWidget->setStyleSheet("background-color: #fdf6ec;");
    auto settleLayout = new QVBoxLayout(settleWidget);
    settleLayout->setContentsMargins(20, 20, 20, 20);
    settleLayout->setSpacing(15);

    auto settleTitle = new QLabel("<b>⚡ 充电控制与订单结算</b>");
    settleTitle->setStyleSheet("font-size: 16pt; color: #e6a23c; font-weight: bold;");
    settleLayout->addWidget(settleTitle);

    chargingStatusLabel = new QLabel;
    chargingStatusLabel->setStyleSheet("background-color: #ffffff; padding: 15px; border-radius: 8px; border: 1px solid #f3d19e; color: #e6a23c; font-weight: bold; font-size: 14px;");
    chargingStatusLabel->setWordWrap(true);
    settleLayout->addWidget(chargingStatusLabel);

    startChargingBtn = new QPushButton("开始充电");
    startChargingBtn->setStyleSheet("background-color: #67c23a; color: white; font-weight: bold; padding: 12px; border-radius: 6px;");
    settleOrderBtn = new QPushButton("结束充电并结算订单");
    settleOrderBtn->setStyleSheet("background-color: #f56c6c; color: white; font-weight: bold; padding: 12px; border-radius: 6px;");
    auto backHomeBtn = new QPushButton("返回首页");
    backHomeBtn->setObjectName("secondaryBtn");

    settleLayout->addWidget(startChargingBtn);
    settleLayout->addWidget(settleOrderBtn);
    settleLayout->addWidget(backHomeBtn);
    settleLayout->addStretch();
    subStack->addWidget(settleWidget);

    connect(backHomeBtn, &QPushButton::clicked, this, &OrderPage::requestBackHome);
}

void OrderPage::refreshOrders(int userId) {
    auto a = PlatformService::orders(userId);
    orderTable->setRowCount(a.size());
    QStringList ss = { "预约中", "充电中", "已完成", "已取消" };

    for (int i = 0; i < a.size(); i++) {
        auto m = a[i].toMap();
        int stVal = m["status"].toInt();
        QStringList v = {
            m["station_name"].toString(),
            m["charger_code"].toString(),
            ss.value(stVal),
            QString::number(m["energy"].toDouble(), 'f', 2) + " kWh",
            "¥ " + QString::number(m["amount"].toDouble(), 'f', 2),
            m["created_at"].toString()
        };
        for (int j = 0; j < v.size(); j++) {
            auto item = new QTableWidgetItem(v[j]);
            if (i == 0 && (stVal == 0 || stVal == 1)) {
                item->setForeground(QColor("#52c41a"));
                QFont f = item->font(); f.setBold(true); item->setFont(f);
            }
            orderTable->setItem(i, j, item);
        }
        orderTable->item(i, 0)->setData(Qt::UserRole, m["id"]);
        orderTable->item(i, 0)->setData(Qt::UserRole + 1, stVal);
    }
}

void OrderPage::updateChargingStatus(int userId) {
    bool hasActive = false;
    for (auto &o : PlatformService::orders(userId)) {
        auto z = o.toMap();
        int stVal = z["status"].toInt();
        if (stVal == 1) {
            hasActive = true;
            int sec = QDateTime::fromString(z["start_time"].toString(), "yyyy-MM-dd HH:mm:ss").secsTo(QDateTime::currentDateTime()) * 60;
            double en = z["power"].toDouble() * sec / 3600.;
            double cost = en * z["price"].toDouble();
            chargingStatusLabel->setText(QString("⚡ [充电中 - 60x演示加速]<br>• 充电时长：%1 秒<br>• 累计电量：%2 kWh<br>• 实时费用：¥ %3").arg(sec).arg(en, 0, 'f', 2).arg(cost, 0, 'f', 2));
            break;
        } else if (stVal == 0) {
            hasActive = true;
            chargingStatusLabel->setText("⏳ 状态：已预约电桩，请点击上方“开始充电”以启动计时。");
            break;
        }
    }
    if (!hasActive) {
        chargingStatusLabel->setText("✅ 当前暂无活跃的预约或充电订单。");
    }
}