#include "OrderListPage.h"
#include "OrderTicketDialog.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QMessageBox>
#include "core/service/PlatformService.h"

OrderListPage::OrderListPage(QWidget *parent) : QWidget(parent)
{
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(15, 15, 15, 15);

    layout->addWidget(new QLabel("<b>历史订单（点击选中后可弹窗查看小票明细）</b>"));

    orderTable = new QTableWidget;
    setupTable();
    layout->addWidget(orderTable);

    viewDetailsBtn = new QPushButton("查看订单小票 / 去结算");
    layout->addWidget(viewDetailsBtn);

    connect(viewDetailsBtn, &QPushButton::clicked, this, &OrderListPage::onViewDetails);
    connect(orderTable, &QTableWidget::doubleClicked, this, &OrderListPage::onViewDetails);
}

void OrderListPage::setupTable()
{
    QStringList headers = { "电站", "电桩", "状态", "电量", "金额", "时间" };
    orderTable->setColumnCount(headers.size());
    orderTable->setHorizontalHeaderLabels(headers);
    orderTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    orderTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    orderTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    orderTable->verticalHeader()->setVisible(false);
}

void OrderListPage::loadOrders(int userId)
{
    currentUserId = userId;
    auto list = PlatformService::orders(userId);
    orderTable->setRowCount(list.size());
    QStringList statusStrList = { "预约中", "充电中", "已完成", "已取消" };

    for (int i = 0; i < list.size(); i++)
    {
        auto m = list[i].toMap();
        int stVal = m["status"].toInt();
        QStringList v = {
            m["station_name"].toString(),
            m["charger_code"].toString(),
            statusStrList.value(stVal),
            QString::number(m["energy"].toDouble(), 'f', 2) + " kWh",
            "¥ " + QString::number(m["amount"].toDouble(), 'f', 2),
            m["created_at"].toString()
        };

        for (int j = 0; j < v.size(); j++)
        {
            auto item = new QTableWidgetItem(v[j]);
            if (i == 0 && (stVal == 0 || stVal == 1)) {
                item->setForeground(QColor("#52c41a"));
                QFont f = item->font();
                f.setBold(true);
                item->setFont(f);
            }
            orderTable->setItem(i, j, item);
        }
        orderTable->item(i, 0)->setData(Qt::UserRole, m["id"]);
        orderTable->item(i, 0)->setData(Qt::UserRole + 1, stVal);
    }
}

void OrderListPage::onViewDetails()
{
    auto x = orderTable->currentItem();
    if (!x) {
        QMessageBox::information(this, "提示", "请选择一条订单");
        return;
    }

    int orderId = orderTable->item(x->row(), 0)->data(Qt::UserRole).toInt();
    int stVal = orderTable->item(x->row(), 0)->data(Qt::UserRole + 1).toInt();

    if (stVal == 0 || stVal == 1) {
        emit goToSettleRequested();
    } else {
        // 选中历史订单：弹窗展示模拟纸质小票明细
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