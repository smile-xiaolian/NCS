#include "SettlePage.h"
#include <QMessageBox>
#include <QDateTime>
#include <QFrame>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include "core/service/PlatformService.h"

static QString formatHms(int seconds)
{
    seconds = qMax(0, seconds);
    return QString("%1:%2:%3")
        .arg(seconds / 3600, 2, 10, QLatin1Char('0'))
        .arg((seconds % 3600) / 60, 2, 10, QLatin1Char('0'))
        .arg(seconds % 60, 2, 10, QLatin1Char('0'));
}

static QWidget *createMetricCard(const QString &title, QLabel *&valueLabel, const QString &unit)
{
    auto *card = new QFrame;
    card->setStyleSheet(
        "QFrame {"
        "  background: #f8fafc;"
        "  border: 1px solid #e8eef4;"
        "  border-radius: 12px;"
        "}"
    );
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(8, 10, 8, 10);
    layout->setSpacing(3);
    layout->setAlignment(Qt::AlignCenter);

    auto *titleLabel = new QLabel(title);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("font-size: 11px; color: #94a3b8; border: none; background: transparent;");

    valueLabel = new QLabel("0.00");
    valueLabel->setAlignment(Qt::AlignCenter);
    valueLabel->setStyleSheet("font-size: 16px; font-weight: 700; color: #0f172a; border: none; background: transparent;");

    auto *unitLabel = new QLabel(unit);
    unitLabel->setAlignment(Qt::AlignCenter);
    unitLabel->setStyleSheet("font-size: 10px; color: #94a3b8; border: none; background: transparent;");

    layout->addWidget(titleLabel);
    layout->addWidget(valueLabel);
    layout->addWidget(unitLabel);
    return card;
}

SettlePage::SettlePage(QWidget *parent) : QWidget(parent)
{
    setStyleSheet("background-color: #f4f7fb; font-family: 'Microsoft YaHei', sans-serif;");

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    auto *stationCard = new QFrame;
    stationCard->setStyleSheet(
        "QFrame {"
        "  background-color: #ffffff;"
        "  border: 1px solid #e2e8f0;"
        "  border-radius: 14px;"
        "}"
    );
    auto *stationLayout = new QVBoxLayout(stationCard);
    stationLayout->setContentsMargins(16, 14, 16, 14);
    stationLayout->setSpacing(6);

    stationNameLabel = new QLabel(QStringLiteral("正在获取电站信息..."));
    stationNameLabel->setStyleSheet("font-size: 16px; font-weight: 700; color: #0f172a; border: none; background: transparent;");

    auto *subInfoLayout = new QHBoxLayout;
    chargerCodeLabel = new QLabel(QStringLiteral("电桩：-"));
    chargerTypeLabel = new QLabel(QStringLiteral("功率：-"));
    priceLabel = new QLabel(QStringLiteral("单价：-"));
    const QString subStyle = "font-size: 12px; color: #64748b; border: none; background: transparent;";
    chargerCodeLabel->setStyleSheet(subStyle);
    chargerTypeLabel->setStyleSheet(subStyle);
    priceLabel->setStyleSheet(subStyle);
    subInfoLayout->addWidget(chargerCodeLabel);
    subInfoLayout->addWidget(chargerTypeLabel);
    subInfoLayout->addWidget(priceLabel);
    stationLayout->addWidget(stationNameLabel);
    stationLayout->addLayout(subInfoLayout);
    mainLayout->addWidget(stationCard);

    auto *progressCard = new QFrame;
    progressCard->setStyleSheet(
        "QFrame {"
        "  background-color: #ffffff;"
        "  border: 1px solid #e2e8f0;"
        "  border-radius: 18px;"
        "}"
    );
    auto *progressLayout = new QVBoxLayout(progressCard);
    progressLayout->setContentsMargins(16, 18, 16, 16);
    progressLayout->setSpacing(12);

    progressWidget = new ChargingProgressWidget;
    progressLayout->addWidget(progressWidget, 0, Qt::AlignCenter);

    statusTipLabel = new QLabel(QStringLiteral("等待启动充电"));
    statusTipLabel->setAlignment(Qt::AlignCenter);
    statusTipLabel->setWordWrap(true);
    statusTipLabel->setStyleSheet(
        "QLabel {"
        "  font-size: 12px; font-weight: 600; color: #0284c7;"
        "  background: #e0f2fe; border: none; border-radius: 10px;"
        "  padding: 8px 12px;"
        "}"
    );
    progressLayout->addWidget(statusTipLabel);

    auto *metricsLayout = new QGridLayout;
    metricsLayout->setHorizontalSpacing(8);
    metricsLayout->setVerticalSpacing(8);
    metricsLayout->addWidget(createMetricCard(QStringLiteral("已充电量"), energyDetailLabel, QStringLiteral("度")), 0, 0);
    metricsLayout->addWidget(createMetricCard(QStringLiteral("实时费用"), costDetailLabel, QStringLiteral("元")), 0, 1);
    metricsLayout->addWidget(createMetricCard(QStringLiteral("充电时长"), durationDetailLabel, QStringLiteral("HH:MM:SS")), 1, 0);
    metricsLayout->addWidget(createMetricCard(QStringLiteral("实时功率"), powerDetailLabel, QStringLiteral("kW")), 1, 1);
    progressLayout->addLayout(metricsLayout);
    mainLayout->addWidget(progressCard, 1);

    startChargingBtn = new QPushButton(QStringLiteral("开始充电"));
    startChargingBtn->setCursor(Qt::PointingHandCursor);
    startChargingBtn->setStyleSheet(
        "QPushButton { background-color: #10b981; color: white; font-weight: 700; font-size: 14px; padding: 12px; border-radius: 10px; border: none; }"
        "QPushButton:hover { background-color: #059669; }"
        "QPushButton:pressed { background-color: #047857; }"
        "QPushButton:disabled { background-color: #cbd5e1; color: #94a3b8; }"
    );

    settleOrderBtn = new QPushButton(QStringLiteral("结束充电并结算"));
    settleOrderBtn->setCursor(Qt::PointingHandCursor);
    settleOrderBtn->setStyleSheet(
        "QPushButton { background-color: #ef4444; color: white; font-weight: 700; font-size: 14px; padding: 12px; border-radius: 10px; border: none; }"
        "QPushButton:hover { background-color: #dc2626; }"
        "QPushButton:pressed { background-color: #b91c1c; }"
    );

    backHomeBtn = new QPushButton(QStringLiteral("返回首页"));
    backHomeBtn->setObjectName("secondaryBtn");
    backHomeBtn->setCursor(Qt::PointingHandCursor);

    mainLayout->addWidget(startChargingBtn);
    mainLayout->addWidget(settleOrderBtn);
    mainLayout->addWidget(backHomeBtn);

    connect(startChargingBtn, &QPushButton::clicked, this, &SettlePage::onStartCharging);
    connect(settleOrderBtn, &QPushButton::clicked, this, &SettlePage::onSettleOrder);
    connect(backHomeBtn, &QPushButton::clicked, this, &SettlePage::backToHomeRequested);
    connect(&timer, &QTimer::timeout, this, &SettlePage::updateChargingStatus);
    timer.start(1000);
}

void SettlePage::setUserId(int userId)
{
    currentUserId = userId;
    updateChargingStatus();
}

void SettlePage::onStartCharging()
{
    QString errorMsg;
    if (PlatformService::start(currentUserId, &errorMsg)) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("已成功开始充电！"));
        updateChargingStatus();
    } else {
        QMessageBox::warning(this, QStringLiteral("提示"),
                             errorMsg.isEmpty() ? QStringLiteral("启动失败，请确认是否有有效预约订单") : errorMsg);
    }
}

void SettlePage::onSettleOrder()
{
    if (QMessageBox::question(this, QStringLiteral("确认结算"),
                              QStringLiteral("确定要结束充电并结算吗？"),
                              QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::No) != QMessageBox::Yes) {
        return;
    }

    QString errorMsg;
    if (PlatformService::settle(currentUserId, &errorMsg)) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("结算完成！感谢您的使用。"));
        resetToIdleState();
        emit settleSuccess();
    } else {
        QMessageBox::warning(this, QStringLiteral("提示"),
                             errorMsg.isEmpty() ? QStringLiteral("当前没有进行中的订单可结算") : errorMsg);
    }
}

void SettlePage::resetToIdleState()
{
    stationNameLabel->setText(QStringLiteral("当前暂无活跃订单"));
    chargerCodeLabel->setText(QStringLiteral("电桩：-"));
    chargerTypeLabel->setText(QStringLiteral("功率：-"));
    priceLabel->setText(QStringLiteral("单价：-"));

    progressWidget->setProgress(0.0);
    progressWidget->setMode(ChargingProgressWidget::Mode::Idle);
    statusTipLabel->setText(QStringLiteral("无进行中订单"));
    statusTipLabel->setStyleSheet(
        "QLabel { font-size: 12px; font-weight: 600; color: #64748b; background: #f1f5f9;"
        " border: none; border-radius: 10px; padding: 8px 12px; }"
    );

    energyDetailLabel->setText("0.00");
    costDetailLabel->setText("0.00");
    durationDetailLabel->setText("00:00:00");
    powerDetailLabel->setText("0.0");
    startChargingBtn->setEnabled(true);
}

void SettlePage::updateChargingStatus()
{
    if (!isVisible() || !currentUserId) return;

    bool hasActive = false;
    for (auto &order : PlatformService::orders(currentUserId)) {
        const auto z = order.toMap();
        const int stVal = z["status"].toInt();
        if (stVal != 0 && stVal != 1) continue;

        hasActive = true;
        stationNameLabel->setText(z["station_name"].toString());
        chargerCodeLabel->setText(QStringLiteral("电桩 %1").arg(z["charger_code"].toString()));
        chargerTypeLabel->setText(QStringLiteral("功率 %1 kW").arg(z["power"].toDouble(), 0, 'f', 1));
        priceLabel->setText(QStringLiteral("¥ %1 /度").arg(z["price"].toDouble(), 0, 'f', 2));
        powerDetailLabel->setText(QString::number(z["power"].toDouble(), 'f', 1));

        if (stVal == 1) {
            startChargingBtn->setEnabled(false);
            const int sec = QDateTime::fromString(z["start_time"].toString(), "yyyy-MM-dd HH:mm:ss")
                                .secsTo(QDateTime::currentDateTime()) * 60;
            double energy = z["power"].toDouble() * sec / 3600.0;
            const double targetMaxEnergy = 30.0;
            double percent = (energy / targetMaxEnergy) * 100.0;

            if (percent >= 100.0) {
                percent = 100.0;
                energy = targetMaxEnergy;
                const double maxCost = energy * z["price"].toDouble();
                const int maxSec = qMin(sec, int(targetMaxEnergy * 3600.0 / z["power"].toDouble()));
                progressWidget->setProgress(100.0);
                progressWidget->setMode(ChargingProgressWidget::Mode::Full);
                statusTipLabel->setText(QStringLiteral("电池已充满，计费已锁定，请结算订单"));
                statusTipLabel->setStyleSheet(
                    "QLabel { font-size: 12px; font-weight: 600; color: #047857; background: #d1fae5;"
                    " border: none; border-radius: 10px; padding: 8px 12px; }"
                );
                energyDetailLabel->setText(QString::number(energy, 'f', 2));
                costDetailLabel->setText(QString::number(maxCost, 'f', 2));
                durationDetailLabel->setText(formatHms(maxSec));
            } else {
                progressWidget->setProgress(percent);
                progressWidget->setMode(ChargingProgressWidget::Mode::Charging);
                statusTipLabel->setText(QStringLiteral("正在充电（演示加速 60x）"));
                statusTipLabel->setStyleSheet(
                    "QLabel { font-size: 12px; font-weight: 600; color: #047857; background: #ecfdf5;"
                    " border: none; border-radius: 10px; padding: 8px 12px; }"
                );
                energyDetailLabel->setText(QString::number(energy, 'f', 2));
                costDetailLabel->setText(QString::number(energy * z["price"].toDouble(), 'f', 2));
                durationDetailLabel->setText(formatHms(sec));
            }
        } else {
            startChargingBtn->setEnabled(true);
            progressWidget->setProgress(0.0);
            progressWidget->setMode(ChargingProgressWidget::Mode::Reserved);
            statusTipLabel->setText(QStringLiteral("已预约电桩，点击下方开始充电"));
            statusTipLabel->setStyleSheet(
                "QLabel { font-size: 12px; font-weight: 600; color: #b45309; background: #fffbeb;"
                " border: none; border-radius: 10px; padding: 8px 12px; }"
            );
            energyDetailLabel->setText("0.00");
            costDetailLabel->setText("0.00");
            durationDetailLabel->setText("00:00:00");
        }
        break;
    }

    if (!hasActive)
        resetToIdleState();
}
