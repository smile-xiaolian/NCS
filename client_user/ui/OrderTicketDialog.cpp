#include "OrderTicketDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>

OrderTicketDialog::OrderTicketDialog(const QVariantMap &orderData, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("订单结算小票明细");
    setFixedSize(360, 520);
    setStyleSheet("QDialog { background-color: #f4f6f8; }");
    setupUi(orderData);
}

void OrderTicketDialog::setupUi(const QVariantMap &data)
{
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // 小票白色卡片背景
    auto ticketCard = new QFrame(this);
    ticketCard->setStyleSheet(
        "QFrame { background-color: #ffffff; border-radius: 12px; border: 1px dashed #dcdfe6; }"
    );
    auto cardLayout = new QVBoxLayout(ticketCard);
    cardLayout->setContentsMargins(20, 25, 20, 25);
    cardLayout->setSpacing(8);

    auto headerLabel = new QLabel("⚡ NCS 智能充电服务小票 ⚡");
    headerLabel->setStyleSheet("font-size: 15pt; font-weight: bold; color: #2b4c7e; border: none;");
    headerLabel->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(headerLabel);

    auto subHeader = new QLabel("----------------------------------------");
    subHeader->setStyleSheet("color: #909399; border: none;");
    subHeader->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(subHeader);

    auto addRow = [cardLayout](const QString &label, const QString &val, bool highlight = false) {
        auto row = new QHBoxLayout();
        auto lLabel = new QLabel(label);
        auto lVal = new QLabel(val);
        lLabel->setStyleSheet("color: #606266; font-size: 12px; border: none;");
        
        if (highlight) {
            lVal->setStyleSheet("color: #f56c6c; font-size: 14px; font-weight: bold; border: none;");
        } else {
            lVal->setStyleSheet("color: #303133; font-size: 12px; font-weight: bold; border: none;");
        }
        lVal->setAlignment(Qt::AlignRight);
        
        row->addWidget(lLabel);
        row->addWidget(lVal);
        cardLayout->addLayout(row);
    };

    int orderId = data["id"].toInt();
    int stVal = data["status"].toInt();
    QString statusStr = (stVal == 2) ? "已完成结算" : ((stVal == 3) ? "已取消" : "进行中");

    addRow("订单编号：", QString("#%1").arg(orderId, 6, 10, QChar('0')));
    addRow("订单状态：", statusStr);
    addRow("充电电站：", data["station_name"].toString());
    addRow("充电电桩：", data["charger_code"].toString());
    addRow("实时功率：", QString::number(data["power"].toDouble(), 'f', 1) + " kW");
    addRow("计费单价：", QString::number(data["price"].toDouble(), 'f', 2) + " 元/度");
    addRow("开始时间：", data["start_time"].toString());
    addRow("结束时间：", data["end_time"].toString().isEmpty() ? "-" : data["end_time"].toString());
    
    auto line = new QLabel("----------------------------------------");
    line->setStyleSheet("color: #909399; border: none;");
    line->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(line);

    addRow("累计已充电量：", QString::number(data["energy"].toDouble(), 'f', 2) + " kWh");
    addRow("消费总金额：", "¥ " + QString::number(data["amount"].toDouble(), 'f', 2), true);

    cardLayout->addStretch();
    mainLayout->addWidget(ticketCard);

    auto closeBtn = new QPushButton("关闭并返回", this);
    closeBtn->setStyleSheet(
        "QPushButton { background-color: #409eff; color: white; border-radius: 6px; padding: 10px; font-weight: bold; }"
        "QPushButton:hover { background-color: #66b1ff; }"
    );
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    mainLayout->addWidget(closeBtn);
}