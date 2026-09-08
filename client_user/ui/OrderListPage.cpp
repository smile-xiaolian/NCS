#include "OrderListPage.h"
#include "OrderTicketDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QMouseEvent>
#include "core/service/PlatformService.h"

OrderListPage::OrderListPage(QWidget *parent) : QWidget(parent)
{
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(10);

    auto tipLabel = new QLabel("<b>我的订单（点击卡片查看小票明细或去结算）</b>");
    tipLabel->setStyleSheet("font-size: 14px; color: #1e293b;");
    mainLayout->addWidget(tipLabel);

    // 滚动区域布局
    scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("QScrollArea { background: transparent; }");

    cardContainerWidget = new QWidget();
    cardContainerWidget->setStyleSheet("background: transparent;");
    cardContainerLayout = new QVBoxLayout(cardContainerWidget);
    cardContainerLayout->setContentsMargins(0, 0, 0, 0);
    cardContainerLayout->setSpacing(12);
    cardContainerLayout->addStretch();

    scrollArea->setWidget(cardContainerWidget);
    mainLayout->addWidget(scrollArea);
}

void OrderListPage::loadOrders(int userId)
{
    currentUserId = userId;

    // 清空旧卡片
    QLayoutItem *item;
    while ((item = cardContainerLayout->takeAt(0)) != nullptr) {
        if (item->widget()) delete item->widget();
        delete item;
    }

    auto list = PlatformService::orders(userId);
    for (const auto &var : list) {
        QVariantMap m = var.toMap();
        QWidget *card = createOrderCard(m);
        cardContainerLayout->addWidget(card);
    }

    cardContainerLayout->addStretch();
}

QWidget* OrderListPage::createOrderCard(const QVariantMap &m)
{
    int orderId = m["id"].toInt();
    int stVal = m["status"].toInt(); // 0: 预约中, 1: 充电中, 2: 已完成, 3: 已取消
    QString stationName = m["station_name"].toString();
    QString chargerCode = m["charger_code"].toString();
    double energy = m["energy"].toDouble();
    double amount = m["amount"].toDouble();
    QString createdAt = m["created_at"].toString();

    // 状态样式映射
    QString statusStr = "未知";
    QString statusColor = "#94a3b8";
    if (stVal == 0) { statusStr = "预约中"; statusColor = "#f59e0b"; }
    else if (stVal == 1) { statusStr = "充电中"; statusColor = "#10b981"; }
    else if (stVal == 2) { statusStr = "已完成"; statusColor = "#3b82f6"; }
    else if (stVal == 3) { statusStr = "已取消"; statusColor = "#ef4444"; }

    auto cardWidget = new QWidget();
    cardWidget->setCursor(Qt::PointingHandCursor);
    cardWidget->setStyleSheet(
        "QWidget {"
        "   background-color: #ffffff;"
        "   border: 1px solid #e2e8f0;"
        "   border-radius: 12px;"
        "}"
        "QWidget:hover {"
        "   border-color: #3b82f6;"
        "   background-color: #f8fafc;"
        "}"
    );

    auto cardLayout = new QVBoxLayout(cardWidget);
    cardLayout->setContentsMargins(15, 12, 15, 12);
    cardLayout->setSpacing(6);

    // 顶部：电站名与状态
    auto topLayout = new QHBoxLayout();
    auto nameLabel = new QLabel(stationName);
    nameLabel->setStyleSheet("font-size: 15px; font-weight: bold; color: #0f172a; border: none; background: transparent;");

    auto statusLabel = new QLabel(QString("<font color='%1'>● <b>%2</b></font>").arg(statusColor, statusStr));
    statusLabel->setStyleSheet("font-size: 13px; border: none; background: transparent;");

    topLayout->addWidget(nameLabel, 1);
    topLayout->addWidget(statusLabel);
    cardLayout->addLayout(topLayout);

    // 中部：枪号与时间
    auto midLayout = new QHBoxLayout();
    auto codeLabel = new QLabel(QString("电桩：<b>%1</b>").arg(chargerCode));
    codeLabel->setStyleSheet("font-size: 12px; color: #64748b; border: none; background: transparent;");

    auto timeLabel = new QLabel(createdAt);
    timeLabel->setStyleSheet("font-size: 11px; color: #94a3b8; border: none; background: transparent;");

    midLayout->addWidget(codeLabel);
    midLayout->addStretch();
    midLayout->addWidget(timeLabel);
    cardLayout->addLayout(midLayout);

    // 底部：电量与金额
    auto bottomLayout = new QHBoxLayout();
    auto energyLabel = new QLabel(QString("用电量：<b>%1 kWh</b>").arg(energy, 0, 'f', 2));
    energyLabel->setStyleSheet("font-size: 12px; color: #475569; border: none; background: transparent;");

    auto amountLabel = new QLabel(QString("金额：<font color='#ef4444'><b>¥ %1</b></font>").arg(amount, 0, 'f', 2));
    amountLabel->setStyleSheet("font-size: 14px; border: none; background: transparent;");

    bottomLayout->addWidget(energyLabel);
    bottomLayout->addStretch();
    bottomLayout->addWidget(amountLabel);
    cardLayout->addLayout(bottomLayout);

    cardWidget->installEventFilter(this);
    cardWidget->setProperty("orderId", orderId);
    cardWidget->setProperty("orderStatus", stVal);

    return cardWidget;
}

bool OrderListPage::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonRelease) {
        auto mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            QVariant idProp = watched->property("orderId");
            QVariant stProp = watched->property("orderStatus");
            if (idProp.isValid() && stProp.isValid()) {
                handleOrderClick(idProp.toInt(), stProp.toInt());
                return true;
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

void OrderListPage::handleOrderClick(int orderId, int status)
{
    if (status == 0 || status == 1) {
        emit goToSettleRequested(); // 活跃订单跳转结算页[cite: 16]
    } else {
        // 历史订单打开纸质小票弹窗[cite: 16]
        auto ordersList = PlatformService::orders(currentUserId);
        for (auto &o : ordersList) {
            auto m = o.toMap();
            if (m["id"].toInt() == orderId) {
                OrderTicketDialog dlg(m, this);
                dlg.exec();
                break;
            }
        }
    }
}
