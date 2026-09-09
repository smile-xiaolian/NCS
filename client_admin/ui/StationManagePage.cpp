#include "StationManagePage.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QSpinBox>
#include <QColor>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
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

// 新增/编辑充电站对话框
class StationEditDialog : public QDialog
{
public:
    explicit StationEditDialog(const QVariantMap &edit = {}, QWidget *parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle(edit.isEmpty() ? QStringLiteral("新增充电站")
                                      : QStringLiteral("编辑充电站"));
        auto *form = new QFormLayout(this);

        mNameEdit = new QLineEdit(edit.value(QStringLiteral("name")).toString());
        form->addRow(QStringLiteral("站名："), mNameEdit);

        mAddressEdit = new QLineEdit(edit.value(QStringLiteral("address")).toString());
        form->addRow(QStringLiteral("地址："), mAddressEdit);

        mLongitudeSpin = new QDoubleSpinBox;
        mLongitudeSpin->setRange(-180.0, 180.0);
        mLongitudeSpin->setDecimals(6);
        mLongitudeSpin->setValue(edit.value(QStringLiteral("longitude")).toDouble());
        form->addRow(QStringLiteral("经度："), mLongitudeSpin);

        mLatitudeSpin = new QDoubleSpinBox;
        mLatitudeSpin->setRange(-90.0, 90.0);
        mLatitudeSpin->setDecimals(6);
        mLatitudeSpin->setValue(edit.value(QStringLiteral("latitude")).toDouble());
        form->addRow(QStringLiteral("纬度："), mLatitudeSpin);

        mPriceSpin = new QDoubleSpinBox;
        mPriceSpin->setRange(0.01, 99.0);
        mPriceSpin->setDecimals(2);
        mPriceSpin->setSuffix(QStringLiteral(" 元/度"));
        mPriceSpin->setValue(edit.value(QStringLiteral("price")).toDouble() > 0.0
                                 ? edit.value(QStringLiteral("price")).toDouble()
                                 : 1.2);
        form->addRow(QStringLiteral("单价："), mPriceSpin);

        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("保存"));
        buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
        connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        form->addRow(buttons);
        resize(740, 620);
        setMinimumSize(640, 500);
    }

    QString name() const { return mNameEdit->text().trimmed(); }
    QString address() const { return mAddressEdit->text().trimmed(); }
    double longitude() const { return mLongitudeSpin->value(); }
    double latitude() const { return mLatitudeSpin->value(); }
    double price() const { return mPriceSpin->value(); }

private:
    QLineEdit *mNameEdit = nullptr;
    QLineEdit *mAddressEdit = nullptr;
    QDoubleSpinBox *mLongitudeSpin = nullptr;
    QDoubleSpinBox *mLatitudeSpin = nullptr;
    QDoubleSpinBox *mPriceSpin = nullptr;
};

StationManagePage::StationManagePage(QWidget *parent)
    : QWidget(parent)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(14);
    root->addWidget(ncs::pageHeading(QStringLiteral("充电站管理"),
                                     QStringLiteral("维护站点基础信息,并联动查看站内电桩")));

    auto *splitter = new QSplitter(Qt::Vertical);
    splitter->setChildrenCollapsible(false);
    splitter->setHandleWidth(12);

    const ncs::Panel stationPanel =
        ncs::titledPanel(QStringLiteral("充电站列表"), this,
                         QStringLiteral("选中电站后下方联动显示其电桩明细"));

    auto *addButton = new QPushButton(QStringLiteral("新增充电站"));
    addButton->setObjectName(QStringLiteral("accent"));
    auto *editButton = new QPushButton(QStringLiteral("编辑"));
    auto *removeButton = new QPushButton(QStringLiteral("删除"));
    removeButton->setObjectName(QStringLiteral("danger"));
    auto *refreshButton = new QPushButton(QStringLiteral("刷新"));
    for (auto *button : {addButton, editButton, removeButton, refreshButton}) {
        stationPanel.header->addWidget(button);
    }

    mStationTable = new QTableWidget;
    setupTable(mStationTable, {QStringLiteral("充电站"), QStringLiteral("地址"),
                               QStringLiteral("单价（元/度）"), QStringLiteral("经度"),
                               QStringLiteral("纬度"), QStringLiteral("桩总数"),
                               QStringLiteral("空闲桩")});
    stationPanel.body->addWidget(mStationTable, 1);
    splitter->addWidget(stationPanel.card);

    const ncs::Panel detailPanel =
        ncs::titledPanel(QStringLiteral("电桩明细"), this);
    mDetailTitle = detailPanel.title;
    mChargerTable = new QTableWidget;
    setupTable(mChargerTable, {QStringLiteral("桩编号"), QStringLiteral("类型"),
                               QStringLiteral("功率（kW）"), QStringLiteral("状态"),
                               QStringLiteral("累计充电次数"), QStringLiteral("累计时长（分）")});
    detailPanel.body->addWidget(mChargerTable, 1);
    splitter->addWidget(detailPanel.card);

    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);
    root->addWidget(splitter, 1);

    connect(mStationTable, &QTableWidget::itemSelectionChanged, this, [this] {
        updateChargerDetail(mStationTable->currentRow());
    });
    connect(addButton, &QPushButton::clicked, this, [this] { addStation(); });
    connect(editButton, &QPushButton::clicked, this, [this] { editStation(); });
    connect(removeButton, &QPushButton::clicked, this, [this] { removeStation(); });
    connect(refreshButton, &QPushButton::clicked, this, [this] { refresh(); });

    refresh();
}

void StationManagePage::refresh()
{
    fillStationTable();
}

void StationManagePage::fillStationTable()
{
    const QVariantList stations = PlatformService::stations();
    mStationTable->clearContents();
    mStationTable->setRowCount(stations.size());

    int row = 0;
    for (const QVariant &item : stations) {
        const QVariantMap station = item.toMap();
        const int stationId = station.value(QStringLiteral("id")).toInt();
        const QVariantMap detail = PlatformService::station(stationId);

        QTableWidgetItem *first = new QTableWidgetItem(
            station.value(QStringLiteral("name")).toString());
        first->setData(Qt::UserRole, stationId);
        mStationTable->setItem(row, 0, first);
        mStationTable->setItem(row, 1, new QTableWidgetItem(
            station.value(QStringLiteral("address")).toString()));
        mStationTable->setItem(row, 2, new QTableWidgetItem(
            ncs::number(station.value(QStringLiteral("price")).toDouble())));
        mStationTable->setItem(row, 3, new QTableWidgetItem(
            ncs::number(station.value(QStringLiteral("longitude")).toDouble(), 6)));
        mStationTable->setItem(row, 4, new QTableWidgetItem(
            ncs::number(station.value(QStringLiteral("latitude")).toDouble(), 6)));
        mStationTable->setItem(row, 5, new QTableWidgetItem(
            QString::number(detail.value(QStringLiteral("total")).toInt())));
        QTableWidgetItem *idleItem = new QTableWidgetItem(
            QString::number(detail.value(QStringLiteral("idle")).toInt()));
        idleItem->setForeground(QColor(QStringLiteral("#1E9E62")));
        mStationTable->setItem(row, 6, idleItem);
        ++row;
    }

    if (stations.isEmpty()) {
        mDetailTitle->setText(QStringLiteral("暂无充电站"));
        mChargerTable->clearContents();
        mChargerTable->setRowCount(0);
    } else {
        mStationTable->selectRow(0);
        updateChargerDetail(0);
    }
}

void StationManagePage::updateChargerDetail(int stationRow)
{
    if (stationRow < 0 || !mStationTable->item(stationRow, 0)) {
        mDetailTitle->setText(QStringLiteral("请选择充电站"));
        mChargerTable->clearContents();
        mChargerTable->setRowCount(0);
        return;
    }
    const int stationId = mStationTable->item(stationRow, 0)->data(Qt::UserRole).toInt();
    const QString stationName = mStationTable->item(stationRow, 0)->text();

    const QVariantList chargers = PlatformService::chargers(stationId);
    mChargerTable->clearContents();
    mChargerTable->setRowCount(chargers.size());

    int row = 0;
    for (const QVariant &item : chargers) {
        const QVariantMap charger = item.toMap();
        mChargerTable->setItem(row, 0, new QTableWidgetItem(
            charger.value(QStringLiteral("code")).toString()));
        mChargerTable->setItem(row, 1, new QTableWidgetItem(
            charger.value(QStringLiteral("type")).toString()));
        mChargerTable->setItem(row, 2, new QTableWidgetItem(
            ncs::number(charger.value(QStringLiteral("power")).toDouble(), 1)));
        const int status = charger.value(QStringLiteral("status")).toInt();
        QTableWidgetItem *statusItem = new QTableWidgetItem(
            ncs::chargerStatusText(status));
        QColor statusColor(QStringLiteral("#8A94A6"));
        switch (status) {
        case 0: statusColor = QColor(QStringLiteral("#1E9E62")); break;
        case 1: statusColor = QColor(QStringLiteral("#E8890C")); break;
        case 2: statusColor = QColor(QStringLiteral("#D64545")); break;
        default: break;
        }
        statusItem->setForeground(statusColor);
        mChargerTable->setItem(row, 3, statusItem);
        mChargerTable->setItem(row, 4, new QTableWidgetItem(
            QString::number(charger.value(QStringLiteral("total_count")).toInt())));
        mChargerTable->setItem(row, 5, new QTableWidgetItem(
            QString::number(charger.value(QStringLiteral("total_minutes")).toInt())));
        ++row;
    }
    mDetailTitle->setText(
        QStringLiteral("「%1」电桩明细（共 %2 台）").arg(stationName).arg(chargers.size()));
}

QVariantMap StationManagePage::selectedStation() const
{
    const int row = mStationTable->currentRow();
    if (row < 0 || !mStationTable->item(row, 0)) {
        return {};
    }
    const int stationId = mStationTable->item(row, 0)->data(Qt::UserRole).toInt();
    return PlatformService::station(stationId);
}

void StationManagePage::showNoSelection() const
{
    ncs::info(const_cast<StationManagePage *>(this),
              QStringLiteral("提示"), QStringLiteral("请先在表格中选择一行"));
}

void StationManagePage::addStation()
{
    StationEditDialog dialog({}, this);
    ncs::fitToScreen(&dialog, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    QString error;
    if (!PlatformService::saveStation(0, dialog.name(), dialog.address(),
                                      dialog.longitude(), dialog.latitude(),
                                      dialog.price(), &error)) {
        ncs::warning(this, QStringLiteral("新增失败"), error);
        return;
    }
    refresh();
}

void StationManagePage::editStation()
{
    const QVariantMap station = selectedStation();
    if (station.isEmpty()) {
        showNoSelection();
        return;
    }
    StationEditDialog dialog(station, this);
    ncs::fitToScreen(&dialog, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    QString error;
    if (!PlatformService::saveStation(station.value(QStringLiteral("id")).toInt(),
                                      dialog.name(), dialog.address(),
                                      dialog.longitude(), dialog.latitude(),
                                      dialog.price(), &error)) {
        ncs::warning(this, QStringLiteral("保存失败"), error);
        return;
    }
    refresh();
}

void StationManagePage::removeStation()
{
    const QVariantMap station = selectedStation();
    if (station.isEmpty()) {
        showNoSelection();
        return;
    }
    const QString name = station.value(QStringLiteral("name")).toString();
    if (!ncs::confirm(this, QStringLiteral("删除确认"),
                      QStringLiteral("确定删除充电站 %1 吗？").arg(name))) {
        return;
    }
    QString error;
    if (!PlatformService::deleteStation(station.value(QStringLiteral("id")).toInt(),
                                        &error)) {
        ncs::warning(this, QStringLiteral("删除失败"), error);
        return;
    }
    refresh();
}
