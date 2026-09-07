#include "SettlePage.h"
#include <QMessageBox>
#include <QDateTime>
#include <QFrame>
#include <QtMath>
#include "core/service/PlatformService.h"

// ---------------------------------------------------------------------
// ChargingProgressWidget 实现：绘制圆环进度条
// ---------------------------------------------------------------------
ChargingProgressWidget::ChargingProgressWidget(QWidget *parent) : QWidget(parent)
{
    setMinimumSize(180, 180);
    setMaximumSize(180, 180);
}

void ChargingProgressWidget::setProgress(double progress)
{
    m_progress = qBound(0.0, progress, 100.0);
    update(); // 触发重绘
}

void ChargingProgressWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int width = this->width();
    int height = this->height();
    int side = qMin(width, height);
    
    int strokeWidth = 12;
    QRectF outerRect(strokeWidth / 2.0 + 2, strokeWidth / 2.0 + 2, 
                     side - strokeWidth - 4, side - strokeWidth - 4);

    // 1. 底层灰色轨道环
    QPen bgPen(QColor("#e4e7ed"), strokeWidth, Qt::SolidLine, Qt::RoundCap);
    painter.setPen(bgPen);
    painter.drawEllipse(outerRect);

    // 2. 绿色/深绿进度环
    if (m_progress > 0) {
        QColor progressColor = (m_progress >= 100.0) ? QColor("#059669") : QColor("#10b981");
        QPen progressPen(progressColor, strokeWidth, Qt::SolidLine, Qt::RoundCap);
        painter.setPen(progressPen);
        
        int startAngle = 90 * 16;
        int spanAngle = -static_cast<int>(m_progress * 3.6 * 16);
        painter.drawArc(outerRect, startAngle, spanAngle);
    }
}

// ---------------------------------------------------------------------
// SettlePage 实现：自动停充逻辑与界面联动
// ---------------------------------------------------------------------
SettlePage::SettlePage(QWidget *parent) : QWidget(parent)
{
    setStyleSheet("background-color: #f8fafc; font-family: 'Microsoft YaHei', sans-serif;");

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(12);

    // 1. 顶部：电站与电桩详细信息卡片
    auto stationCard = new QFrame();
    stationCard->setStyleSheet(
        "QFrame {"
        "   background-color: #ffffff;"
        "   border: 1px solid #e2e8f0;"
        "   border-radius: 12px;"
        "}"
    );
    auto stationLayout = new QVBoxLayout(stationCard);
    stationLayout->setContentsMargins(15, 12, 15, 12);
    stationLayout->setSpacing(4);

    stationNameLabel = new QLabel("正在获取电站信息...");
    stationNameLabel->setStyleSheet("font-size: 15px; font-weight: bold; color: #1e293b; border: none; background: transparent;");
    
    auto subInfoLayout = new QHBoxLayout();
    chargerCodeLabel = new QLabel("电桩：-");
    chargerTypeLabel = new QLabel("类型：-");
    priceLabel = new QLabel("单价：- 元/度");

    QString subStyle = "font-size: 12px; color: #64748b; border: none; background: transparent;";
    chargerCodeLabel->setStyleSheet(subStyle);
    chargerTypeLabel->setStyleSheet(subStyle);
    priceLabel->setStyleSheet(subStyle);

    subInfoLayout->addWidget(chargerCodeLabel);
    subInfoLayout->addWidget(chargerTypeLabel);
    subInfoLayout->addWidget(priceLabel);

    stationLayout->addWidget(stationNameLabel);
    stationLayout->addLayout(subInfoLayout);
    mainLayout->addWidget(stationCard);

    // 2. 中部：充电进度圆环与仪表板
    auto progressCard = new QFrame();
    progressCard->setStyleSheet(
        "QFrame {"
        "   background-color: #ffffff;"
        "   border: 1px solid #e2e8f0;"
        "   border-radius: 16px;"
        "}"
    );
    auto progressLayout = new QVBoxLayout(progressCard);
    progressLayout->setContentsMargins(15, 20, 15, 20);
    progressLayout->setSpacing(10);
    progressLayout->setAlignment(Qt::AlignCenter);

    auto ringContainer = new QWidget();
    ringContainer->setFixedSize(180, 180);
    
    progressWidget = new ChargingProgressWidget(ringContainer);
    progressWidget->setGeometry(0, 0, 180, 180);

    progressPercentLabel = new QLabel("0.0%", ringContainer);
    progressPercentLabel->setGeometry(0, 0, 180, 180);
    progressPercentLabel->setAlignment(Qt::AlignCenter);
    progressPercentLabel->setStyleSheet("font-size: 28px; font-weight: bold; color: #1e293b; background: transparent;");

    progressLayout->addWidget(ringContainer, 0, Qt::AlignCenter);

    statusTipLabel = new QLabel("等待启动充电");
    statusTipLabel->setAlignment(Qt::AlignCenter);
    statusTipLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #0284c7;");
    progressLayout->addWidget(statusTipLabel);

    auto metricsLayout = new QHBoxLayout();
    
    auto createMetricItem = [](const QString &title, QLabel* &valueLabel, const QString &unit) -> QWidget* {
        auto w = new QWidget();
        auto l = new QVBoxLayout(w);
        l->setContentsMargins(0, 0, 0, 0);
        l->setSpacing(2);
        l->setAlignment(Qt::AlignCenter);

        auto tLbl = new QLabel(title);
        tLbl->setStyleSheet("font-size: 11px; color: #94a3b8;");
        tLbl->setAlignment(Qt::AlignCenter);

        valueLabel = new QLabel("0.0");
        valueLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #0f172a;");
        valueLabel->setAlignment(Qt::AlignCenter);

        auto uLbl = new QLabel(unit);
        uLbl->setStyleSheet("font-size: 10px; color: #94a3b8;");
        uLbl->setAlignment(Qt::AlignCenter);

        l->addWidget(tLbl);
        l->addWidget(valueLabel);
        l->addWidget(uLbl);
        return w;
    };

    metricsLayout->addWidget(createMetricItem("已充电量", energyDetailLabel, "度 (kWh)"));
    metricsLayout->addWidget(createMetricItem("实时费用", costDetailLabel, "元 (¥)"));
    metricsLayout->addWidget(createMetricItem("累计时长", durationDetailLabel, "秒 (s)"));

    progressLayout->addLayout(metricsLayout);
    mainLayout->addWidget(progressCard, 1);

    // 3. 底部：控制与结算按钮区
    startChargingBtn = new QPushButton("开始充电");
    startChargingBtn->setStyleSheet(
        "QPushButton { background-color: #10b981; color: white; font-weight: bold; font-size: 14px; padding: 12px; border-radius: 8px; border: none; }"
        "QPushButton:hover { background-color: #059669; }"
        "QPushButton:pressed { background-color: #047857; }"
        "QPushButton:disabled { background-color: #cbd5e1; color: #94a3b8; }"
    );

    settleOrderBtn = new QPushButton("结束充电并结算订单");
    settleOrderBtn->setStyleSheet(
        "QPushButton { background-color: #ef4444; color: white; font-weight: bold; font-size: 14px; padding: 12px; border-radius: 8px; border: none; }"
        "QPushButton:hover { background-color: #dc2626; }"
        "QPushButton:pressed { background-color: #b91c1c; }"
    );

    backHomeBtn = new QPushButton("返回首页");
    backHomeBtn->setObjectName("secondaryBtn");

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
        QMessageBox::information(this, "提示", "已成功开始充电！");
        updateChargingStatus();
    } else {
        QMessageBox::warning(this, "提示", errorMsg.isEmpty() ? "启动失败，请确认是否有有效预约订单" : errorMsg);
    }
}

void SettlePage::onSettleOrder()
{
    QString errorMsg;
    if (PlatformService::settle(currentUserId, &errorMsg)) {
        QMessageBox::information(this, "提示", "结算完成！感谢您的使用。");
        resetToIdleState();
        emit settleSuccess();
    } else {
        QMessageBox::warning(this, "提示", errorMsg.isEmpty() ? "当前没有进行中的订单可结算" : errorMsg);
    }
}

void SettlePage::resetToIdleState()
{
    stationNameLabel->setText("当前暂无活跃订单");
    chargerCodeLabel->setText("电桩：-");
    chargerTypeLabel->setText("类型：-");
    priceLabel->setText("单价：- 元/度");

    progressWidget->setProgress(0.0);
    progressPercentLabel->setText("0.0%");
    statusTipLabel->setText("无进行中订单");
    statusTipLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #64748b;");

    energyDetailLabel->setText("0.00");
    costDetailLabel->setText("0.00");
    durationDetailLabel->setText("0");

    startChargingBtn->setEnabled(true);
}

void SettlePage::updateChargingStatus()
{
    if (!isVisible() || !currentUserId) return;

    bool hasActive = false;
    for (auto &o : PlatformService::orders(currentUserId))
    {
        auto z = o.toMap();
        int stVal = z["status"].toInt(); // 0: 预约中, 1: 充电中
        
        if (stVal == 0 || stVal == 1)
        {
            hasActive = true;
            stationNameLabel->setText(z["station_name"].toString());
            chargerCodeLabel->setText(QString("电桩编号：%1").arg(z["charger_code"].toString()));
            chargerTypeLabel->setText(QString("功率：%1 kW").arg(z["power"].toDouble(), 0, 'f', 1));
            priceLabel->setText(QString("单价：¥ %1").arg(z["price"].toDouble(), 0, 'f', 2));

            if (stVal == 1) { // 正在充电中
                startChargingBtn->setEnabled(false); // 已经在充电中，禁用开始按钮

                // 计算时间与电量
                int sec = QDateTime::fromString(z["start_time"].toString(), "yyyy-MM-dd HH:mm:ss").secsTo(QDateTime::currentDateTime()) * 60;
                double en = z["power"].toDouble() * sec / 3600.0;
                
                // 设定充满上限电量 targetMaxEnergy 为 30 kWh
                double targetMaxEnergy = 30.0; 
                double percent = (en / targetMaxEnergy) * 100.0;

                // 核心逻辑：当电量达到或超过 100% 时，自动停止增加扣费和电量
                if (percent >= 100.0) {
                    percent = 100.0;
                    en = targetMaxEnergy; // 锁定为满电电量
                    double maxCost = en * z["price"].toDouble(); // 锁定最大费用

                    progressWidget->setProgress(100.0);
                    progressPercentLabel->setText("100.0%");
                    
                    statusTipLabel->setText("🔋 电池已充满，已自动停止充电！请点击下方按钮结算");
                    statusTipLabel->setStyleSheet("font-size: 13px; font-weight: bold; color: #059669;");

                    energyDetailLabel->setText(QString::number(en, 'f', 2));
                    costDetailLabel->setText(QString::number(maxCost, 'f', 2));
                    // 记录充满时的时长
                    int maxSec = qMin(sec, static_cast<int>(targetMaxEnergy * 3600.0 / z["power"].toDouble()));
                    durationDetailLabel->setText(QString::number(maxSec));

                } else {
                    // 未充满时，正常实时推进
                    double cost = en * z["price"].toDouble();

                    progressWidget->setProgress(percent);
                    progressPercentLabel->setText(QString("%1%").arg(percent, 0, 'f', 1));
                    
                    statusTipLabel->setText("⚡ 正在快充中 (60x加速)...");
                    statusTipLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #10b981;");

                    energyDetailLabel->setText(QString::number(en, 'f', 2));
                    costDetailLabel->setText(QString::number(cost, 'f', 2));
                    durationDetailLabel->setText(QString::number(sec));
                }

            } else if (stVal == 0) { // 已预约但未开启充电
                startChargingBtn->setEnabled(true);
                progressWidget->setProgress(0.0);
                progressPercentLabel->setText("0.0%");
                statusTipLabel->setText("已预约电桩，请点击下方“开始充电”");
                statusTipLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #f59e0b;");

                energyDetailLabel->setText("0.00");
                costDetailLabel->setText("0.00");
                durationDetailLabel->setText("0");
            }
            break;
        }
    }

    if (!hasActive) {
        resetToIdleState();
    }
}
