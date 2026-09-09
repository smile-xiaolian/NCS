#include "UserManagePage.h"

#include <QDialog>
#include <QColor>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

#include "core/service/PlatformService.h"
#include "FormatUtil.h"
#include "UiKit.h"

namespace {

void setupTable(QTableWidget *table, const QStringList &headers)
{
    table->setColumnCount(headers.size());
    table->setHorizontalHeaderLabels(headers);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->horizontalHeader()->setMinimumHeight(42);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setShowGrid(false);
    table->setAlternatingRowColors(true);
    table->verticalHeader()->setVisible(false);
    table->verticalHeader()->setDefaultSectionSize(40);
}

} // namespace

// 用户订单历史弹窗
class UserOrdersDialog : public QDialog
{
public:
    UserOrdersDialog(int userId, const QString &phone, QWidget *parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle(QStringLiteral("用户订单历史 - %1").arg(ncs::maskedPhone(phone)));
        resize(1720, 960);
        setMinimumSize(1000, 560);

        auto *root = new QVBoxLayout(this);
        auto *table = new QTableWidget;
        setupTable(table, {QStringLiteral("订单号"), QStringLiteral("充电站"),
                           QStringLiteral("电桩编号"), QStringLiteral("状态"),
                           QStringLiteral("下单时间"), QStringLiteral("开始时间"),
                           QStringLiteral("结束时间"), QStringLiteral("电量（度）"),
                           QStringLiteral("金额（元）")});

        const QVariantList orders = PlatformService::orders(userId);
        table->setRowCount(orders.size());
        int row = 0;
        for (const QVariant &item : orders) {
            const QVariantMap order = item.toMap();
            table->setItem(row, 0, new QTableWidgetItem(
                QString::number(order.value(QStringLiteral("id")).toInt())));
            table->setItem(row, 1, new QTableWidgetItem(
                order.value(QStringLiteral("station_name")).toString()));
            table->setItem(row, 2, new QTableWidgetItem(
                order.value(QStringLiteral("charger_code")).toString()));
            table->setItem(row, 3, new QTableWidgetItem(
                ncs::orderStatusText(order.value(QStringLiteral("status")).toInt())));
            table->setItem(row, 4, new QTableWidgetItem(
                order.value(QStringLiteral("created_at")).toString()));
            table->setItem(row, 5, new QTableWidgetItem(
                order.value(QStringLiteral("start_time")).toString()));
            table->setItem(row, 6, new QTableWidgetItem(
                order.value(QStringLiteral("end_time")).toString()));
            table->setItem(row, 7, new QTableWidgetItem(
                ncs::number(order.value(QStringLiteral("energy")).toDouble())));
            table->setItem(row, 8, new QTableWidgetItem(
                ncs::number(order.value(QStringLiteral("amount")).toDouble())));
            ++row;
        }
        root->addWidget(table, 1);

        int finished = 0;
        double totalAmount = 0.0;
        for (const QVariant &item : orders) {
            const QVariantMap order = item.toMap();
            if (order.value(QStringLiteral("status")).toInt() == 2) {
                ++finished;
                totalAmount += order.value(QStringLiteral("amount")).toDouble();
            }
        }
        auto *summary = new QLabel(
            QStringLiteral("共 %1 笔订单（已完成 %2 笔，金额合计 ¥%3）")
                .arg(orders.size())
                .arg(finished)
                .arg(ncs::number(totalAmount)));
        summary->setObjectName(QStringLiteral("panelSub"));
        root->addWidget(summary);
    }
};

UserManagePage::UserManagePage(QWidget *parent)
    : QWidget(parent)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(14);
    root->addWidget(ncs::pageHeading(QStringLiteral("用户管理"),
                                     QStringLiteral("查询注册用户并管理账号状态")));

    const ncs::Panel panel =
        ncs::titledPanel(QStringLiteral("用户列表"), this);

    mSearchEdit = new QLineEdit;
    mSearchEdit->setPlaceholderText(QStringLiteral("输入手机号后回车或点搜索"));
    mSearchEdit->setMinimumWidth(240);
    mSearchEdit->setClearButtonEnabled(true);
    auto *searchButton = new QPushButton(QStringLiteral("搜索"));
    searchButton->setObjectName(QStringLiteral("accent"));
    auto *refreshButton = new QPushButton(QStringLiteral("刷新"));
    mFreezeButton = new QPushButton(QStringLiteral("冻结 / 解冻选中用户"));
    mFreezeButton->setObjectName(QStringLiteral("danger"));
    auto *orderButton = new QPushButton(QStringLiteral("查看订单历史"));
    auto *searchCaption = new QLabel(QStringLiteral("手机号"), panel.card);
    searchCaption->setObjectName(QStringLiteral("filterLabel"));
    panel.header->addWidget(searchCaption);
    panel.header->addWidget(mSearchEdit);
    panel.header->addSpacing(10);
    panel.header->addWidget(searchButton);
    panel.header->addWidget(refreshButton);
    panel.header->addSpacing(6);
    panel.header->addWidget(mFreezeButton);
    panel.header->addWidget(orderButton);

    mTable = new QTableWidget;
    setupTable(mTable, {QStringLiteral("ID"), QStringLiteral("手机号"),
                        QStringLiteral("昵称"), QStringLiteral("余额（元）"),
                        QStringLiteral("状态"), QStringLiteral("注册时间")});
    panel.body->addWidget(mTable, 1);
    root->addWidget(panel.card, 1);

    connect(searchButton, &QPushButton::clicked, this, [this] { refresh(); });
    connect(refreshButton, &QPushButton::clicked, this, [this] { refresh(); });
    connect(mSearchEdit, &QLineEdit::returnPressed, this, [this] { refresh(); });
    connect(mFreezeButton, &QPushButton::clicked, this, [this] { toggleFreeze(); });
    connect(orderButton, &QPushButton::clicked, this, [this] { showOrderHistory(); });

    refresh();
}

void UserManagePage::refresh()
{
    fillTable();
}

void UserManagePage::fillTable()
{
    const QString keyword = mSearchEdit->text().trimmed();
    const QVariantList users = PlatformService::users(keyword);
    mTable->clearContents();
    mTable->setRowCount(users.size());

    int row = 0;
    for (const QVariant &item : users) {
        const QVariantMap user = item.toMap();
        QTableWidgetItem *first = new QTableWidgetItem(
            QString::number(user.value(QStringLiteral("id")).toInt()));
        first->setData(Qt::UserRole, user);
        mTable->setItem(row, 0, first);
        mTable->setItem(row, 1, new QTableWidgetItem(
            ncs::maskedPhone(user.value(QStringLiteral("phone")).toString())));
        mTable->setItem(row, 2, new QTableWidgetItem(
            user.value(QStringLiteral("nickname")).toString()));
        mTable->setItem(row, 3, new QTableWidgetItem(
            ncs::number(user.value(QStringLiteral("balance")).toDouble())));
        const bool active =
            user.value(QStringLiteral("status")).toInt() == 1;
        QTableWidgetItem *statusItem =
            new QTableWidgetItem(active ? QStringLiteral("正常")
                                        : QStringLiteral("冻结"));
        statusItem->setForeground(active ? QColor(QStringLiteral("#1E9E62"))
                                         : QColor(QStringLiteral("#D64545")));
        mTable->setItem(row, 4, statusItem);
        mTable->setItem(row, 5, new QTableWidgetItem(
            user.value(QStringLiteral("created_at")).toString()));
        ++row;
    }
}

QVariantMap UserManagePage::selectedUser() const
{
    const int row = mTable->currentRow();
    if (row < 0 || !mTable->item(row, 0)) {
        return {};
    }
    return mTable->item(row, 0)->data(Qt::UserRole).toMap();
}

void UserManagePage::showNoSelection() const
{
    ncs::info(const_cast<UserManagePage *>(this),
              QStringLiteral("提示"), QStringLiteral("请先在表格中选择一行"));
}

void UserManagePage::toggleFreeze()
{
    const QVariantMap user = selectedUser();
    if (user.isEmpty()) {
        showNoSelection();
        return;
    }
    const int id = user.value(QStringLiteral("id")).toInt();
    const int status = user.value(QStringLiteral("status")).toInt();
    const int nextStatus = status == 1 ? 0 : 1;
    const QString action = nextStatus == 0 ? QStringLiteral("冻结") : QStringLiteral("解冻");

    if (!ncs::confirm(this, action + QStringLiteral("确认"),
                      QStringLiteral("确定%1用户（ID %2）吗？").arg(action).arg(id))) {
        return;
    }
    PlatformService::setUserStatus(id, nextStatus);
    refresh();
}

void UserManagePage::showOrderHistory()
{
    const QVariantMap user = selectedUser();
    if (user.isEmpty()) {
        showNoSelection();
        return;
    }
    UserOrdersDialog dialog(user.value(QStringLiteral("id")).toInt(),
                            user.value(QStringLiteral("phone")).toString(), this);
    ncs::fitToScreen(&dialog, this);
    dialog.exec();
}
