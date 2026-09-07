#ifndef SETTLEPAGE_H
#define SETTLEPAGE_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QVariantMap>

// 1. 自定义高级感圆环充电进度条控件
class ChargingProgressWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ChargingProgressWidget(QWidget *parent = nullptr);
    void setProgress(double progress); // 0.0 ~ 100.0

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    double m_progress = 0.0;
};

// 2. 充电控制与结算主页面
class SettlePage : public QWidget
{
    Q_OBJECT
public:
    explicit SettlePage(QWidget *parent = nullptr);
    void setUserId(int userId);
    void updateChargingStatus();

signals:
    void backToHomeRequested();
    void settleSuccess();

private:
    int currentUserId = 0;
    
    // 顶部卡片：电站与电桩信息
    QLabel *stationNameLabel;
    QLabel *chargerCodeLabel;
    QLabel *chargerTypeLabel;
    QLabel *priceLabel;

    // 中部：圆环进度与详细文字信息
    ChargingProgressWidget *progressWidget;
    QLabel *progressPercentLabel;
    QLabel *energyDetailLabel;
    QLabel *costDetailLabel;
    QLabel *durationDetailLabel;
    QLabel *statusTipLabel;

    // 底部：操作按钮
    QPushButton *startChargingBtn;
    QPushButton *settleOrderBtn;
    QPushButton *backHomeBtn;

    QTimer timer;

    void onStartCharging();
    void onSettleOrder();
    void resetToIdleState();
};

#endif // SETTLEPAGE_H
