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
    setMinimumSize(200, 200);
    setMaximumSize(200, 200);
}

void ChargingProgressWidget::setProgress(double progress)
{
    m_progress = qBound(0.0, progress, 100.0);
    update(); // 触发重绘
}

void ChargingProgressWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing); // 高质量抗锯齿

    int side = qMin(width(), height());
    int strokeWidth = 14;
    QRectF outerRect(strokeWidth / 2.0 + 3, strokeWidth / 2.0 + 3, 
                     side - strokeWidth - 6, side - strokeWidth - 6);

    // 1. 底层轨道 (极简灰色圆环)
    QPen bgPen(QColor("#f1f5f9"), strokeWidth, Qt::SolidLine, Qt::RoundCap); //
    painter.setPen(bgPen);
    painter.drawEllipse(outerRect);

    // 2. 顶层动态渐变进度环 (12点钟方向顺时针推进)
    if (m_progress > 0) {
        QLinearGradient gradient(0, 0, width(), height());
        if (m_progress >= 100.0) {
            gradient.setColorAt(0.0, QColor("#059669")); // 满电深翡翠绿[cite: 15]
            gradient.setColorAt(1.0, QColor("#10b981"));
        } else {
            gradient.setColorAt(0.0, QColor("#34d399")); // 充能极光青绿[cite: 15]
            gradient.setColorAt(1.0, QColor("#10b981"));
        }

        QPen progressPen(QBrush(gradient), strokeWidth, Qt::SolidLine, Qt::RoundCap);
        painter.setPen(progressPen);

        int startAngle = 90 * 16; // 12点钟方向[cite: 15]
        int spanAngle = -static_cast<int>(m_progress * 3.6 * 16); // 顺时针方向[cite: 15]
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
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(14);

    // 1. 顶部电站信息 Card
    auto stationCard = new QFrame();
    stationCard->setStyleSheet(
        "QFrame {"
        "   background-color: #ffffff;"
        "   border: 1px solid #e2e8f0;"
        "   border-radius: 16px;"
        "}"
    );
    auto stationLayout = new QVBoxLayout(stationCard);
    stationLayout->setContentsMargins(16, 14, 16, 14);
    stationLayout->setSpacing(6);

    stationNameLabel = new QLabel("正在获取电站信息...");
    stationNameLabel->setStyleSheet("font-size: 15px; font-weight: bold; color: #0f172a; border: none; background: transparent;");
    
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

    // 2. 中部发光圆环与仪表盘 Card
    auto progressCard = new QFrame();
    progressCard->setStyleSheet(
        "QFrame {"
        "   background-color: #ffffff;"
        "   border: 1px solid #e2e8f0;"
        "   border-radius: 20px;"
        "}"
    );
    auto progressLayout = new QVBoxLayout(progressCard);
    progressLayout->setContentsMargins(16, 24, 16, 24);
    progressLayout->setSpacing(12);
    progressLayout->setAlignment(Qt::AlignCenter);

    auto ringContainer = new QWidget();
    ringContainer->setFixedSize(180, 180);
    
    progressWidget = new ChargingProgressWidget(ringContainer);
    progressWidget->setGeometry(0, 0, 180, 180);

    progressPercentLabel = new QLabel("0.0%", ringContainer);
    progressPercentLabel->setGeometry(0, 0, 180, 180);
    progressPercentLabel->setAlignment(Qt::AlignCenter);
    progressPercentLabel->setStyleSheet("font-size: 30px; font-weight: 800; color: #0f172a; background: transparent;");

    progressLayout->addWidget(ringContainer, 0, Qt::AlignCenter);

    statusTipLabel = new QLabel("等待启动充电");
    statusTipLabel->setAlignment(Qt::AlignCenter);
    statusTipLabel->setStyleSheet("font-size: 13px; font-weight: bold; color: #10b981;");
    progressLayout->addWidget(statusTipLabel);

    // 3 列三位一体数据看板 (用电量 / 金额 / 时长)
    auto metricsLayout = new QHBoxLayout();
    auto createMetricItem = [](const QString &title, QLabel* &valueLabel, const QString &unit) -> QWidget* {
        auto w = new QWidget();
        auto l = new QVBoxLayout(w);
        l->setContentsMargins(0, 0, 0, 0);
        l->setSpacing(2);
        l->setAlignment(Qt::AlignCenter);

        auto tLbl = new QLabel(title);
        tLbl->setStyleSheet("font-size: 11px; color: #64748b;");
        tLbl->setAlignment(Qt::AlignCenter);

        valueLabel = new QLabel("0.00");
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

    // 3. 底部胶囊按钮区
    startChargingBtn = new QPushButton("开始充电", this);
    startChargingBtn->setCursor(Qt::PointingHandCursor);
    startChargingBtn->setStyleSheet(
        "QPushButton { background-color: #10b981; color: white; font-weight: bold; font-size: 14px; padding: 12px; border-radius: 14px; border: none; }"
        "QPushButton:hover { background-color: #059669; }"
        "QPushButton:disabled { background-color: #cbd5e1; color: #94a3b8; }"
    );

    settleOrderBtn = new QPushButton("结束充电并结算订单", this);
    settleOrderBtn->setCursor(Qt::PointingHandCursor);
    settleOrderBtn->setStyleSheet(
        "QPushButton { background-color: #ef4444; color: white; font-weight: bold; font-size: 14px; padding: 12px; border-radius: 14px; border: none; }"
        "QPushButton:hover { background-color: #dc2626; }"
    );

    backHomeBtn = new QPushButton("返回首页", this);
    backHomeBtn->setCursor(Qt::PointingHandCursor);
    backHomeBtn->setStyleSheet(
        "QPushButton { background-color: #f1f5f9; color: #475569; font-weight: bold; font-size: 13px; padding: 10px; border-radius: 14px; border: none; }"
        "QPushButton:hover { background-color: #e2e8f0; }"
    );

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
    for (auto &o : PlatformService::orders(currentUserId)) {
        auto z = o.toMap();
        int stVal = z["status"].toInt(); // 0: 预约中, 1: 充电中
        
        if (stVal == 0 || stVal == 1) {
            hasActive = true;
            stationNameLabel->setText(z["station_name"].toString());
            chargerCodeLabel->setText(QString("电桩编号：%1").arg(z["charger_code"].toString()));
            chargerTypeLabel->setText(QString("功率：%1 kW").arg(z["power"].toDouble(), 0, 'f', 1));
            priceLabel->setText(QString("单价：¥ %1").arg(z["price"].toDouble(), 0, 'f', 2));

            if (stVal == 1) { // 充电中
                startChargingBtn->setEnabled(false);

                int sec = QDateTime::fromString(z["start_time"].toString(), "yyyy-MM-dd HH:mm:ss").secsTo(QDateTime::currentDateTime()) * 60;
                double en = z["power"].toDouble() * sec / 3600.0;
                double targetMaxEnergy = 30.0; // 充满上限[cite: 13]
                double percent = (en / targetMaxEnergy) * 100.0;

                if (percent >= 100.0) { // 充满自动停止计算[cite: 13]
                    percent = 100.0;
                    en = targetMaxEnergy;
                    double maxCost = en * z["price"].toDouble();

                    progressWidget->setProgress(100.0);
                    progressPercentLabel->setText("100%");
                    statusTipLabel->setText("🔋 电池已充满，已自动停止计费");
                    statusTipLabel->setStyleSheet("font-size: 13px; font-weight: bold; color: #059669;");

                    energyDetailLabel->setText(QString::number(en, 'f', 2));
                    costDetailLabel->setText(QString::number(maxCost, 'f', 2));
                    int maxSec = qMin(sec, static_cast<int>(targetMaxEnergy * 3600.0 / z["power"].toDouble()));
                    durationDetailLabel->setText(QString::number(maxSec));
                } else {
                    double cost = en * z["price"].toDouble();
                    progressWidget->setProgress(percent);
                    progressPercentLabel->setText(QString("%1%").arg(percent, 0, 'f', 1));
                    statusTipLabel->setText("⚡ 正在快充中...");
                    statusTipLabel->setStyleSheet("font-size: 13px; font-weight: bold; color: #10b981;");

                    energyDetailLabel->setText(QString::number(en, 'f', 2));
                    costDetailLabel->setText(QString::number(cost, 'f', 2));
                    durationDetailLabel->setText(QString::number(sec));
                }
            }
            break;
        }
    }
    if (!hasActive) resetToIdleState();
}
