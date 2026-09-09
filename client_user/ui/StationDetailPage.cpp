#include "StationDetailPage.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QMouseEvent>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QSequentialAnimationGroup>
#include "core/service/PlatformService.h"


StationDetailPage::StationDetailPage(QWidget *parent) : QWidget(parent)
{
    setStyleSheet("background-color: #f8fafc; font-family: 'Microsoft YaHei', sans-serif;");

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    // 1. 顶部电站标题与极简提示
    titleLabel = new QLabel(this);
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #0f172a; margin-bottom: 2px;");
    mainLayout->addWidget(titleLabel);

    auto detailTip = new QLabel("请点击空闲电桩卡片直接预约（需余额 ≥ 5 元）：", this);
    detailTip->setStyleSheet("color: #64748b; font-size: 12px;");
    mainLayout->addWidget(detailTip);

    // 2. 滚动区域
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

    // 3. 底部操作胶囊栏
    auto bottomBtnLayout = new QHBoxLayout();
    navigateBtn = new QPushButton("一键导航", this);
    navigateBtn->setCursor(Qt::PointingHandCursor);
    navigateBtn->setStyleSheet(
        "QPushButton {"
        "   background-color: #f1f5f9; color: #0f172a; font-weight: bold;"
        "   border-radius: 12px; padding: 10px 16px; border: none;"
        "}"
        "QPushButton:hover { background-color: #e2e8f0; }"
    );

    backBtn = new QPushButton("返回首页", this);
    backBtn->setCursor(Qt::PointingHandCursor);
    backBtn->setStyleSheet(
        "QPushButton {"
        "   background-color: #10b981; color: #ffffff; font-weight: bold;"
        "   border-radius: 12px; padding: 10px 16px; border: none;"
        "}"
        "QPushButton:hover { background-color: #059669; }"
    );

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

    // 清空旧卡片
    QLayoutItem *item;
    while ((item = cardContainerLayout->takeAt(0)) != nullptr) {
        if (item->widget()) delete item->widget();
        delete item;
    }

    auto sInfo = PlatformService::station(stationId);
    titleLabel->setText(sInfo["name"].toString() + " · 选桩");

    auto list = PlatformService::chargers(stationId);
    QList<QWidget*> cardList;

    for (const auto &var : list) {
        QVariantMap z = var.toMap();
        QWidget *card = createChargerCard(z);
        cardContainerLayout->addWidget(card);
        cardList.append(card);
    }
    cardContainerLayout->addStretch();

    // 🌟 触发卡片流式交错淡入与平滑上滑进场动画
    auto seqGroup = new QSequentialAnimationGroup(this);
    for (int i = 0; i < cardList.size(); ++i) {
        auto card = cardList[i];
        auto opacityEffect = new QGraphicsOpacityEffect(card);
        card->setGraphicsEffect(opacityEffect);

        auto parallel = new QParallelAnimationGroup(seqGroup);

        auto fade = new QPropertyAnimation(opacityEffect, "opacity");
        fade->setDuration(280);
        fade->setStartValue(0.0);
        fade->setEndValue(1.0);
        fade->setEasingCurve(QEasingCurve::OutCubic);

        auto move = new QPropertyAnimation(card, "pos");
        move->setDuration(280);
        QPoint finalPos = card->pos();
        move->setStartValue(QPoint(finalPos.x(), finalPos.y() + 20));
        move->setEndValue(finalPos);
        move->setEasingCurve(QEasingCurve::OutCubic);

        parallel->addAnimation(fade);
        parallel->addAnimation(move);
        seqGroup->addAnimation(parallel);
    }
    seqGroup->start(QAbstractAnimation::DeleteWhenStopped);
}

QWidget* StationDetailPage::createChargerCard(const QVariantMap &z)
{
    int chargerId = z["id"].toInt();
    QString code = z["code"].toString();
    QString type = z["type"].toString();
    double power = z["power"].toDouble();
    int stStatus = z["status"].toInt(); // 0: 空闲, 1: 使用中, 2: 故障[cite: 11]

    QString statusStr = "未知";
    QString statusFg = "#64748b";
    QString statusBg = "#f1f5f9";
    if (stStatus == 0)      { statusStr = "空闲"; statusFg = "#10b981"; statusBg = "#ecfdf5"; } //[cite: 11]
    else if (stStatus == 1) { statusStr = "使用中"; statusFg = "#f59e0b"; statusBg = "#fef3c7"; } //[cite: 11]
    else if (stStatus == 2) { statusStr = "故障"; statusFg = "#ef4444"; statusBg = "#fef2f2"; } //[cite: 11]

    auto cardWidget = new QWidget();
    cardWidget->setCursor(stStatus == 0 ? Qt::PointingHandCursor : Qt::ForbiddenCursor); //[cite: 11]
    
    QString hoverStyle = (stStatus == 0) ? "QWidget:hover { border-color: #10b981; background-color: #ffffff; }" : "";
    cardWidget->setStyleSheet(QString(
        "QWidget {"
        "   background-color: #ffffff;"
        "   border: 1px solid #e2e8f0;"
        "   border-radius: 16px;"
        "}"
        "%1"
    ).arg(hoverStyle));

    auto cardLayout = new QVBoxLayout(cardWidget);
    cardLayout->setContentsMargins(16, 14, 16, 14);
    cardLayout->setSpacing(8);

    // 1. 枪号与 Pill 胶囊标签
    auto topLayout = new QHBoxLayout();
    auto codeLabel = new QLabel(QString("桩号：%1").arg(code));
    codeLabel->setStyleSheet("font-size: 15px; font-weight: bold; color: #0f172a; border: none; background: transparent;");

    auto statusLabel = new QLabel(QString(" %1 ").arg(statusStr));
    statusLabel->setStyleSheet(QString(
        "font-size: 11px; font-weight: bold; color: %1; background-color: %2; "
        "border-radius: 8px; padding: 2px 8px; border: none;"
    ).arg(statusFg, statusBg));

    topLayout->addWidget(codeLabel, 1);
    topLayout->addWidget(statusLabel);
    cardLayout->addLayout(topLayout);

    // 2. 类型与功率参数
    auto bottomLayout = new QHBoxLayout();
    auto typeLabel = new QLabel(QString("类型：<b>%1</b>").arg(type));
    typeLabel->setStyleSheet("font-size: 12px; color: #64748b; border: none; background: transparent;");

    auto powerLabel = new QLabel(QString("输出功率：<font color='#10b981'><b>%1 kW</b></font>").arg(power, 0, 'f', 1));
    powerLabel->setStyleSheet("font-size: 12px; color: #64748b; border: none; background: transparent;");

    bottomLayout->addWidget(typeLabel);
    bottomLayout->addStretch();
    bottomLayout->addWidget(powerLabel);
    cardLayout->addLayout(bottomLayout);

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
