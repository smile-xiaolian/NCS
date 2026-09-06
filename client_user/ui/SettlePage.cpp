#include "SettlePage.h"
#include <QVBoxLayout>
#include <QMessageBox>
#include <QDateTime>
#include "core/service/PlatformService.h"

SettlePage::SettlePage(QWidget *parent) : QWidget(parent)
{
    setStyleSheet("background-color: #fdf6ec;");

    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(15);

    auto settleTitle = new QLabel("<b>充电控制与订单结算</b>");
    settleTitle->setStyleSheet("font-size: 16pt; color: #e6a23c; font-weight: bold;");
    layout->addWidget(settleTitle);

    chargingStatusLabel = new QLabel;
    chargingStatusLabel->setStyleSheet("background-color: #ffffff; padding: 15px; border-radius: 8px; border: 1px solid #f3d19e; color: #e6a23c; font-weight: bold; font-size: 14px;");
    chargingStatusLabel->setWordWrap(true);
    layout->addWidget(chargingStatusLabel);

    startChargingBtn = new QPushButton("开始充电");
    startChargingBtn->setStyleSheet("background-color: #67c23a; color: white; font-weight: bold; padding: 12px; border-radius: 6px;");

    settleOrderBtn = new QPushButton("结束充电并结算订单");
    settleOrderBtn->setStyleSheet("background-color: #f56c6c; color: white; font-weight: bold; padding: 12px; border-radius: 6px;");

    backHomeBtn = new QPushButton("返回首页");
    backHomeBtn->setObjectName("secondaryBtn");

    layout->addWidget(startChargingBtn);
    layout->addWidget(settleOrderBtn);
    layout->addWidget(backHomeBtn);
    layout->addStretch();

    connect(startChargingBtn, &QPushButton::clicked, this, &SettlePage::onStartCharging);
    connect(settleOrderBtn, &QPushButton::clicked, this, &SettlePage::onSettleOrder);
    connect(backHomeBtn, &QPushButton::clicked, this, &SettlePage::backToHomeRequested);

    connect(&timer, &QTimer::timeout, this, &SettlePage::updateChargingStatus);
    timer.start(1000);
}

void SettlePage::setUserId(int userId)
{
    currentUserId = userId;
}

void SettlePage::onStartCharging()
{
    QString errorMsg;
    if (PlatformService::start(currentUserId, &errorMsg)) {
        QMessageBox::information(this, "提示", "已成功开始充电！");
    } else {
        QMessageBox::warning(this, "提示", errorMsg.isEmpty() ? "启动失败，请确认是否有有效预约订单" : errorMsg);
    }
}

void SettlePage::onSettleOrder()
{
    QString errorMsg;
    if (PlatformService::settle(currentUserId, &errorMsg)) {
        QMessageBox::information(this, "提示", "结算完成！如余额不足扣，剩余部分记入欠费并提醒。");
        emit settleSuccess();
    } else {
        QMessageBox::warning(this, "提示", errorMsg.isEmpty() ? "当前没有进行中的订单可结算" : errorMsg);
    }
}

void SettlePage::updateChargingStatus()
{
    if (!isVisible() || !currentUserId) return;

    bool hasActive = false;
    for (auto &o : PlatformService::orders(currentUserId))
    {
        auto z = o.toMap();
        int stVal = z["status"].toInt();
        if (stVal == 1) // 充电中
        {
            hasActive = true;
            int sec = QDateTime::fromString(z["start_time"].toString(), "yyyy-MM-dd HH:mm:ss").secsTo(QDateTime::currentDateTime()) * 60;
            double en = z["power"].toDouble() * sec / 3600.;
            double cost = en * z["price"].toDouble();
            chargingStatusLabel->setText(
                QString("[充电中 - 60x演示加速]<br>"
                        "• 充电时长：%1 秒<br>"
                        "• 累计电量：%2 度 (kWh)<br>"
                        "• 实时费用：¥ %3")
                    .arg(sec).arg(en, 0, 'f', 2).arg(cost, 0, 'f', 2)
            );
            break;
        }
        else if (stVal == 0) // 预约中
        {
            hasActive = true;
            chargingStatusLabel->setText("状态：已预约电桩，请点击上方“开始充电”以启动计时。");
            break;
        }
    }
    if (!hasActive) {
        chargingStatusLabel->setText("✅ 当前暂无活跃的预约或充电订单。");
    }
}