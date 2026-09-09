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

    // 状态色彩样式表映射[cite: 7]
    QString statusStr = "未知";
    QString statusFg = "#64748b";
    QString statusBg = "#f1f5f9";
    if (stVal == 0)      { statusStr = "📌 预约中"; statusFg = "#d97706"; statusBg = "#fef3c7"; }
    else if (stVal == 1) { statusStr = "⚡ 充电中"; statusFg = "#10b981"; statusBg = "#ecfdf5"; }
    else if (stVal == 2) { statusStr = "已完成"; statusFg = "#3b82f6"; statusBg = "#eff6ff"; }
    else if (stVal == 3) { statusStr = "已取消"; statusFg = "#ef4444"; statusBg = "#fef2f2"; }

    auto cardWidget = new QWidget();
    cardWidget->setCursor(Qt::PointingHandCursor);
    cardWidget->setStyleSheet(
        "QWidget {"
        "   background-color: #ffffff;"
        "   border: 1px solid #e2e8f0;"
        "   border-radius: 16px;"
        "}"
        "QWidget:hover {"
        "   border-color: #10b981;"
        "}"
    );

    auto cardLayout = new QVBoxLayout(cardWidget);
    cardLayout->setContentsMargins(16, 14, 16, 14);
    cardLayout->setSpacing(10);

    // 1. 顶部：站点与状态徽章[cite: 7]
    auto topLayout = new QHBoxLayout();
    auto nameLabel = new QLabel(stationName);
    nameLabel->setStyleSheet("font-size: 15px; font-weight: bold; color: #0f172a; border: none; background: transparent;");

    auto statusLabel = new QLabel(QString(" %1 ").arg(statusStr));
    statusLabel->setStyleSheet(QString(
        "font-size: 11px; font-weight: bold; color: %1; background-color: %2; "
        "border-radius: 8px; padding: 3px 8px; border: none;"
    ).arg(statusFg, statusBg));

    topLayout->addWidget(nameLabel, 1);
    topLayout->addWidget(statusLabel);
    cardLayout->addLayout(topLayout);

    // 2. 细分割线[cite: 7]
    auto line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("color: #f8fafc;");
    cardLayout->addWidget(line);

    // 3. 中部：电桩编号与时间[cite: 7]
    auto midLayout = new QHBoxLayout();
    auto codeLabel = new QLabel(QString("充电桩号：%1").arg(chargerCode));
    codeLabel->setStyleSheet("font-size: 12px; color: #64748b; border: none; background: transparent;");

    auto timeLabel = new QLabel(createdAt);
    timeLabel->setStyleSheet("font-size: 11px; color: #94a3b8; border: none; background: transparent;");

    midLayout->addWidget(codeLabel);
    midLayout->addStretch();
    midLayout->addWidget(timeLabel);
    cardLayout->addLayout(midLayout);

    // 4. 底部数据看板：用电量与金额[cite: 7]
    auto bottomLayout = new QHBoxLayout();
    auto energyLabel = new QLabel(QString("已充电量：<b>%1 kWh</b>").arg(energy, 0, 'f', 2));
    energyLabel->setStyleSheet("font-size: 12px; color: #334155; border: none; background: transparent;");

    auto amountLabel = new QLabel(QString("¥ <font size='4'><b>%1</b></font>").arg(amount, 0, 'f', 2));
    amountLabel->setStyleSheet("font-size: 13px; font-weight: bold; color: #0f172a; border: none; background: transparent;");

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
