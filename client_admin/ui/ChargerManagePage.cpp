#include "ChargerManagePage.h"

#include <QComboBox>
#include <QColor>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTableWidget>
#include <QVBoxLayout>

#include "core/service/PlatformService.h"
#include "DeviceLinkHook.h"
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

void fillStationCombo(QComboBox *combo, const QVariantList &stations,
                      int selectedStationId = 0)
{
    combo->clear();
    for (const QVariant &item : stations) {
        const QVariantMap station = item.toMap();
        combo->addItem(station.value(QStringLiteral("name")).toString(),
                       station.value(QStringLiteral("id")).toInt());
    }
    const int index = combo->findData(selectedStationId);
    combo->setCurrentIndex(index >= 0 ? index : 0);
}

} // namespace

// 新增/编辑充电桩对话框
class ChargerEditDialog : public QDialog
{
public:
    ChargerEditDialog(const QVariantList &stations, const QVariantMap &edit,
                      QWidget *parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle(edit.isEmpty() ? QStringLiteral("新增充电桩")
                                      : QStringLiteral("编辑充电桩"));
        auto *form = new QFormLayout(this);

        mStationCombo = new QComboBox;
        fillStationCombo(mStationCombo, stations,
                         edit.value(QStringLiteral("station_id")).toInt());
        form->addRow(QStringLiteral("所属电站："), mStationCombo);

        mCodeEdit = new QLineEdit(edit.value(QStringLiteral("code")).toString());
        mCodeEdit->setPlaceholderText(QStringLiteral("如 P001"));
        form->addRow(QStringLiteral("桩编号："), mCodeEdit);

        mTypeCombo = new QComboBox;
        mTypeCombo->addItems({QStringLiteral("快充"), QStringLiteral("慢充")});
        const int typeIndex = mTypeCombo->findText(
            edit.value(QStringLiteral("type")).toString());
        mTypeCombo->setCurrentIndex(typeIndex >= 0 ? typeIndex : 0);
        form->addRow(QStringLiteral("类型："), mTypeCombo);

        mPowerSpin = new QDoubleSpinBox;
        mPowerSpin->setRange(1.0, 1000.0);
        mPowerSpin->setDecimals(1);
        mPowerSpin->setSuffix(QStringLiteral(" kW"));
        mPowerSpin->setValue(edit.value(QStringLiteral("power")).toDouble() > 0.0
                                 ? edit.value(QStringLiteral("power")).toDouble()
                                 : 7.0);
        form->addRow(QStringLiteral("功率："), mPowerSpin);

        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("保存"));
        buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
        connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        form->addRow(buttons);
        resize(760, 560);
        setMinimumSize(640, 460);
    }

    int stationId() const { return mStationCombo->currentData().toInt(); }
    QString code() const { return mCodeEdit->text().trimmed(); }
    QString type() const { return mTypeCombo->currentText(); }
    double power() const { return mPowerSpin->value(); }

private:
    QComboBox *mStationCombo = nullptr;
    QLineEdit *mCodeEdit = nullptr;
    QComboBox *mTypeCombo = nullptr;
    QDoubleSpinBox *mPowerSpin = nullptr;
};

// 批量建桩对话框
class BatchChargerDialog : public QDialog
{
public:
    explicit BatchChargerDialog(const QVariantList &stations, QWidget *parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle(QStringLiteral("批量建桩"));
        auto *form = new QFormLayout(this);

        mStationCombo = new QComboBox;
        fillStationCombo(mStationCombo, stations);
        form->addRow(QStringLiteral("所属电站："), mStationCombo);

        mTypeCombo = new QComboBox;
        mTypeCombo->addItems({QStringLiteral("快充"), QStringLiteral("慢充")});
        form->addRow(QStringLiteral("类型："), mTypeCombo);

        mPowerSpin = new QDoubleSpinBox;
        mPowerSpin->setRange(1.0, 1000.0);
        mPowerSpin->setDecimals(1);
        mPowerSpin->setSuffix(QStringLiteral(" kW"));
        mPowerSpin->setValue(7.0);
        form->addRow(QStringLiteral("单桩功率："), mPowerSpin);

        mPrefixEdit = new QLineEdit(QStringLiteral("P"));
        mPrefixEdit->setPlaceholderText(QStringLiteral("编号前缀，如 P"));
        form->addRow(QStringLiteral("编号前缀："), mPrefixEdit);

        mCountSpin = new QSpinBox;
        mCountSpin->setRange(1, 99);
        mCountSpin->setValue(5);
        form->addRow(QStringLiteral("数量："), mCountSpin);

        auto *hint = new QLabel(QStringLiteral("编号将自动生成：前缀 + 3 位序号（如 P001）。"));
        hint->setObjectName(QStringLiteral("panelSub"));
        form->addRow(hint);

        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("开始建桩"));
        buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
        connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        form->addRow(buttons);
        resize(760, 640);
        setMinimumSize(640, 520);
    }

    int stationId() const { return mStationCombo->currentData().toInt(); }
    QString type() const { return mTypeCombo->currentText(); }
    double power() const { return mPowerSpin->value(); }
    QString prefix() const { return mPrefixEdit->text().trimmed(); }
    int count() const { return mCountSpin->value(); }

private:
    QComboBox *mStationCombo = nullptr;
    QComboBox *mTypeCombo = nullptr;
    QDoubleSpinBox *mPowerSpin = nullptr;
    QLineEdit *mPrefixEdit = nullptr;
    QSpinBox *mCountSpin = nullptr;
};

ChargerManagePage::ChargerManagePage(QWidget *parent)
    : QWidget(parent)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(14);
    root->addWidget(ncs::pageHeading(QStringLiteral("充电桩管理"),
                                     QStringLiteral("统一维护电站桩资源,支持批量建桩与远程重启")));

    const ncs::Panel panel =
        ncs::titledPanel(QStringLiteral("充电桩列表"), this, QStringLiteral("实时台账"));

    mStationFilter = new QComboBox;
    mStationFilter->setMinimumWidth(180);
    mStatusFilter = new QComboBox;
    mStatusFilter->addItems({QStringLiteral("全部状态"), QStringLiteral("空闲"),
                             QStringLiteral("使用中"), QStringLiteral("故障")});

    auto *stationCaption = new QLabel(QStringLiteral("所属电站"), panel.card);
    stationCaption->setObjectName(QStringLiteral("filterLabel"));
    auto *statusCaption = new QLabel(QStringLiteral("状态"), panel.card);
    statusCaption->setObjectName(QStringLiteral("filterLabel"));
    panel.header->addWidget(stationCaption);
    panel.header->addWidget(mStationFilter);
    panel.header->addSpacing(12);
    panel.header->addWidget(statusCaption);
    panel.header->addWidget(mStatusFilter);
    panel.header->addSpacing(14);

    auto *addButton = new QPushButton(QStringLiteral("新增充电桩"));
    auto *batchButton = new QPushButton(QStringLiteral("批量建桩"));
    addButton->setObjectName(QStringLiteral("accent"));
    batchButton->setObjectName(QStringLiteral("accent"));
    auto *editButton = new QPushButton(QStringLiteral("编辑"));
    auto *removeButton = new QPushButton(QStringLiteral("删除"));
    removeButton->setObjectName(QStringLiteral("danger"));
    auto *restartButton = new QPushButton(QStringLiteral("远程重启"));
    auto *refreshButton = new QPushButton(QStringLiteral("刷新"));
    for (auto *button : {addButton, batchButton, editButton, removeButton,
                         restartButton, refreshButton}) {
        panel.header->addWidget(button);
    }

    mTable = new QTableWidget;
    setupTable(mTable, {QStringLiteral("桩编号"), QStringLiteral("所属电站"),
                        QStringLiteral("类型"), QStringLiteral("功率（kW）"),
                        QStringLiteral("状态"), QStringLiteral("累计充电次数"),
                        QStringLiteral("累计时长（分）")});
    panel.body->addWidget(mTable, 1);
    root->addWidget(panel.card, 1);

    connect(mStationFilter, &QComboBox::currentIndexChanged, this,
            [this](int) { fillTable(); });
    connect(mStatusFilter, &QComboBox::currentIndexChanged, this,
            [this](int) { fillTable(); });
    connect(addButton, &QPushButton::clicked, this, [this] { addCharger(); });
    connect(batchButton, &QPushButton::clicked, this, [this] { addChargersInBatch(); });
    connect(editButton, &QPushButton::clicked, this, [this] { editCharger(); });
    connect(removeButton, &QPushButton::clicked, this, [this] { removeCharger(); });
    connect(restartButton, &QPushButton::clicked, this, [this] { restartCharger(); });
    connect(refreshButton, &QPushButton::clicked, this, [this] { refresh(); });

    refresh();
}

void ChargerManagePage::refresh()
{
    reloadStationFilter();
    fillTable();
}

void ChargerManagePage::reloadStationFilter()
{
    const int selectedStation = mStationFilter->currentData().toInt();
    const QSignalBlocker blocker(mStationFilter);
    mStationFilter->clear();
    mStationFilter->addItem(QStringLiteral("全部电站"), 0);
    for (const QVariant &item : PlatformService::stations()) {
        const QVariantMap station = item.toMap();
        mStationFilter->addItem(station.value(QStringLiteral("name")).toString(),
                                station.value(QStringLiteral("id")).toInt());
    }
    const int index = mStationFilter->findData(selectedStation);
    mStationFilter->setCurrentIndex(index >= 0 ? index : 0);
}

void ChargerManagePage::fillTable()
{
    mTable->setRowCount(0);
    const int stationId = mStationFilter->currentData().toInt();
    const int statusFilter = mStatusFilter->currentIndex(); // 0=全部,1=空闲,2=使用中,3=故障

    const QVariantList chargers = PlatformService::chargers(stationId);
    int row = 0;
    for (const QVariant &item : chargers) {
        const QVariantMap charger = item.toMap();
        if (statusFilter > 0 &&
            charger.value(QStringLiteral("status")).toInt() != statusFilter - 1) {
            continue;
        }
        const int status = charger.value(QStringLiteral("status")).toInt();
        const QString statusText = ncs::chargerStatusText(status);

        mTable->insertRow(row);
        QTableWidgetItem *codeItem = new QTableWidgetItem(
            charger.value(QStringLiteral("code")).toString());
        codeItem->setData(Qt::UserRole, charger);
        mTable->setItem(row, 0, codeItem);
        mTable->setItem(row, 1, new QTableWidgetItem(
            charger.value(QStringLiteral("station_name")).toString()));
        mTable->setItem(row, 2, new QTableWidgetItem(
            charger.value(QStringLiteral("type")).toString()));
        mTable->setItem(row, 3, new QTableWidgetItem(
            ncs::number(charger.value(QStringLiteral("power")).toDouble(), 1)));
        QTableWidgetItem *statusItem = new QTableWidgetItem(statusText);
        QColor statusColor(QStringLiteral("#8A94A6"));
        switch (status) {
        case 0: statusColor = QColor(QStringLiteral("#1E9E62")); break;
        case 1: statusColor = QColor(QStringLiteral("#E8890C")); break;
        case 2: statusColor = QColor(QStringLiteral("#D64545")); break;
        default: break;
        }
        statusItem->setForeground(statusColor);
        mTable->setItem(row, 4, statusItem);
        mTable->setItem(row, 5, new QTableWidgetItem(
            QString::number(charger.value(QStringLiteral("total_count")).toInt())));
        mTable->setItem(row, 6, new QTableWidgetItem(
            QString::number(charger.value(QStringLiteral("total_minutes")).toInt())));
        ++row;
    }
}

QVariantMap ChargerManagePage::selectedCharger() const
{
    const int row = mTable->currentRow();
    if (row < 0 || !mTable->item(row, 0)) {
        return {};
    }
    return mTable->item(row, 0)->data(Qt::UserRole).toMap();
}

void ChargerManagePage::showNoSelection() const
{
    ncs::info(const_cast<ChargerManagePage *>(this),
              QStringLiteral("提示"), QStringLiteral("请先在表格中选择一行"));
}

void ChargerManagePage::addCharger()
{
    ChargerEditDialog dialog(PlatformService::stations(), {}, this);
    ncs::fitToScreen(&dialog, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    QString error;
    if (!PlatformService::saveCharger(0, dialog.stationId(), dialog.code(),
                                      dialog.type(), dialog.power(), &error)) {
        ncs::warning(this, QStringLiteral("新增失败"), error);
        return;
    }
    refresh();
}

void ChargerManagePage::addChargersInBatch()
{
    BatchChargerDialog dialog(PlatformService::stations(), this);
    ncs::fitToScreen(&dialog, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    const QString prefix = dialog.prefix();
    if (prefix.isEmpty()) {
        ncs::warning(this, QStringLiteral("提示"),
                     QStringLiteral("请填写编号前缀"));
        return;
    }
    int success = 0;
    QStringList failed;
    for (int i = 1; i <= dialog.count(); ++i) {
        const QString code =
            prefix + QStringLiteral("%1").arg(i, 3, 10, QChar('0'));
        QString error;
        if (PlatformService::saveCharger(0, dialog.stationId(), code,
                                         dialog.type(), dialog.power(), &error)) {
            ++success;
        } else {
            failed << (code + QStringLiteral("：") + error);
        }
    }
    QString message = QStringLiteral("批量建桩完成：成功 %1 台").arg(success);
    if (!failed.isEmpty()) {
        message += QStringLiteral("\n失败 %1 台（编号已存在等）：\n").arg(failed.size()) +
                   failed.first();
    }
    ncs::info(this, QStringLiteral("批量建桩"), message);
    refresh();
}

void ChargerManagePage::editCharger()
{
    const QVariantMap charger = selectedCharger();
    if (charger.isEmpty()) {
        showNoSelection();
        return;
    }
    ChargerEditDialog dialog(PlatformService::stations(), charger, this);
    ncs::fitToScreen(&dialog, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    QString error;
    if (!PlatformService::saveCharger(charger.value(QStringLiteral("id")).toInt(),
                                      dialog.stationId(), dialog.code(),
                                      dialog.type(), dialog.power(), &error)) {
        ncs::warning(this, QStringLiteral("保存失败"), error);
        return;
    }
    refresh();
}

void ChargerManagePage::removeCharger()
{
    const QVariantMap charger = selectedCharger();
    if (charger.isEmpty()) {
        showNoSelection();
        return;
    }
    const QString code = charger.value(QStringLiteral("code")).toString();
    if (!ncs::confirm(this, QStringLiteral("删除确认"),
                      QStringLiteral("确定删除充电桩 %1 吗？").arg(code))) {
        return;
    }
    QString error;
    if (!PlatformService::deleteCharger(
            charger.value(QStringLiteral("id")).toInt(), &error)) {
        ncs::warning(this, QStringLiteral("删除失败"), error);
        return;
    }
    refresh();
}

void ChargerManagePage::restartCharger()
{
    const QVariantMap charger = selectedCharger();
    if (charger.isEmpty()) {
        showNoSelection();
        return;
    }
    const QString code = charger.value(QStringLiteral("code")).toString();
    if (!ncs::confirm(this, QStringLiteral("远程重启确认"),
                      QStringLiteral("确定远程重启充电桩 %1 吗？"
                                     "重启后故障桩将恢复正常。").arg(code))) {
        return;
    }
    QString error;
    if (!PlatformService::restartCharger(
            charger.value(QStringLiteral("id")).toInt(), &error)) {
        ncs::warning(this, QStringLiteral("重启失败"), error);
        return;
    }
    // 可选联动:若 my_device_link_sim 模拟器平台在线,异步通知其向该桩下发 RemoteReset;
    // 模拟器不在线时静默忽略,不影响上方已完成的数据库逻辑
    ncs::notifySimulatorReset(code);
    ncs::info(this, QStringLiteral("远程重启"),
              QStringLiteral("充电桩 %1 已重启并恢复正常").arg(code));
    refresh();
}
