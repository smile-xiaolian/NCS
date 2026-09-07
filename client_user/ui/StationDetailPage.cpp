#include "StationDetailPage.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QMouseEvent>
#include "core/service/PlatformService.h"

StationDetailPage::StationDetailPage(QWidget *parent) : QWidget(parent)
{
    setStyleSheet("background-color: #f3f0ff;");

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(10);

    // 1. 顶部标题与提示
    titleLabel = new QLabel;
    titleLabel->setStyleSheet("font-size: 15pt; font-weight: bold; color: #5b21b6; margin-bottom: 2px;");
    mainLayout->addWidget(titleLabel);

    auto detailTip = new QLabel("请点击空闲电桩卡片直接预约（余额需 ≥ 5 元）：");
    detailTip->setStyleSheet("color: #6b21a8; font-weight: bold; font-size: 12px;");
    mainLayout->addWidget(detailTip);

    // 2. 替换为卡片滚动列表容器
    scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("QScrollArea { background: transparent; }");

    cardContainerWidget = new QWidget();
    cardContainerWidget->setStyleSheet("background: transparent;");
    cardContainerLayout = new QVBoxLayout(cardContainerWidget);
    cardContainerLayout->setContentsMargins(0, 0, 0, 0);
    cardContainerLayout->setSpacing(12);
    cardContainerLayout->addStretch(); // 底部弹簧置底

    scrollArea->setWidget(cardContainerWidget);
    mainLayout->addWidget(scrollArea);

    // 3. 底部导航与返回按钮区
    auto bottomBtnLayout = new QHBoxLayout();
    navigateBtn = new QPushButton("一键导航");
    navigateBtn->setObjectName("secondaryBtn");

    backBtn = new QPushButton("返回首页");
    backBtn->setObjectName("secondaryBtn");

    bottomBtnLayout->addWidget(navigateBtn);
    bottomBtnLayout->addWidget(backBtn);
    mainLayout->addLayout(bottomBtnLayout);

    connect(navigateBtn, &QPushButton::clicked, this, &StationDetailPage::onNavigate);
    connect(backBtn, &QPushButton::clicked, this, &StationDetailPage::backToHomeRequested);
}

void StationDetailPage::loadStation(int stationId, int userId)
{
    currentStationId = stationId;
    currentUserId = userId;

    // 清空旧卡片（保留最后的弹性垫片）
    QLayoutItem *item;
    while ((item = cardContainerLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }

    auto sInfo = PlatformService::station(stationId);
    titleLabel->setText(sInfo["name"].toString() + " - 电桩列表");

    auto list = PlatformService::chargers(stationId);
    for (const auto &var : list) {
        QVariantMap z = var.toMap();
        QWidget *card = createChargerCard(z);
        cardContainerLayout->addWidget(card);
    }

    cardContainerLayout->addStretch();
}

QWidget* StationDetailPage::createChargerCard(const QVariantMap &z)
{
    int chargerId = z["id"].toInt();
    QString code = z["code"].toString();
    QString type = z["type"].toString();
    double power = z["power"].toDouble();
    int stStatus = z["status"].toInt(); // 0: 空闲, 1: 使用中, 2: 故障

    // 状态字符串与颜色映射
    QString statusStr = "未知";
    QString statusColor = "#909399";
    if (stStatus == 0) {
        statusStr = "空闲";
        statusColor = "#52c41a"; // 绿色
    } else if (stStatus == 1) {
        statusStr = "使用中";
        statusColor = "#fa8c16"; // 橙色
    } else if (stStatus == 2) {
        statusStr = "故障";
        statusColor = "#ff4d4f"; // 红色
    }

    // 1. 外层卡片 Widget
    auto cardWidget = new QWidget();
    cardWidget->setCursor(stStatus == 0 ? Qt::PointingHandCursor : Qt::ForbiddenCursor);
    
    // 只有空闲桩增加 Hover 蓝边框高亮
    QString hoverStyle = (stStatus == 0) ? "QWidget:hover { border-color: #7c3aed; background-color: #fcfaff; }" : "";
    cardWidget->setStyleSheet(QString(
        "QWidget {"
        "   background-color: #ffffff;"
        "   border: 1px solid #e4e7ed;"
        "   border-radius: 12px;"
        "}"
        "%1"
    ).arg(hoverStyle));

    auto cardLayout = new QVBoxLayout(cardWidget);
    cardLayout->setContentsMargins(15, 12, 15, 12);
    cardLayout->setSpacing(6);

    // 2. 第一行：电桩编号与状态标签
    auto topLayout = new QHBoxLayout();
    auto codeLabel = new QLabel(QString("电桩编号：%1").arg(code));
    codeLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #2c3e50; border: none; background: transparent;");

    auto statusLabel = new QLabel(QString("<font color='%1'>● <b>%2</b></font>").arg(statusColor, statusStr));
    statusLabel->setStyleSheet("font-size: 13px; border: none; background: transparent;");

    topLayout->addWidget(codeLabel, 1);
    topLayout->addWidget(statusLabel);
    cardLayout->addLayout(topLayout);

    // 3. 第二行：类型与功率属性
    auto bottomLayout = new QHBoxLayout();
    auto typeLabel = new QLabel(QString("类型：<b>%1</b>").arg(type));
    typeLabel->setStyleSheet("font-size: 12px; color: #606266; border: none; background: transparent;");

    auto powerLabel = new QLabel(QString("输出功率：<font color='#7c3aed'><b>%1 kW</b></font>").arg(power, 0, 'f', 1));
    powerLabel->setStyleSheet("font-size: 12px; color: #606266; border: none; background: transparent;");

    bottomLayout->addWidget(typeLabel);
    bottomLayout->addStretch();
    bottomLayout->addWidget(powerLabel);
    cardLayout->addLayout(bottomLayout);

    // 4. 挂载点击监听与属性数据
    cardWidget->installEventFilter(this);
    cardWidget->setProperty("chargerId", chargerId);
    cardWidget->setProperty("chargerStatus", stStatus);

    return cardWidget;
}

bool StationDetailPage::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonRelease) {
        auto mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            QVariant idProp = watched->property("chargerId");
            QVariant statusProp = watched->property("chargerStatus");

            if (idProp.isValid() && statusProp.isValid()) {
                int chargerId = idProp.toInt();
                int stStatus = statusProp.toInt();

                if (stStatus == 0) { // 空闲状态，发起预约
                    reserveCharger(chargerId);
                } else if (stStatus == 1) {
                    QMessageBox::information(this, "提示", "该电桩目前正在使用中，请选择其他空闲电桩！");
                } else {
                    QMessageBox::warning(this, "提示", "该电桩已被标记为故障，暂停预约！");
                }
                return true;
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

void StationDetailPage::reserveCharger(int chargerId)
{
    QString errorMsg;
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
