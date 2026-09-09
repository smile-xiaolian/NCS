#ifndef SETTLEPAGE_H
#define SETTLEPAGE_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include "ChargingProgressWidget.h"

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

    QLabel *stationNameLabel;
    QLabel *chargerCodeLabel;
    QLabel *chargerTypeLabel;
    QLabel *priceLabel;

    ChargingProgressWidget *progressWidget;
    QLabel *statusTipLabel;
    QLabel *energyDetailLabel;
    QLabel *costDetailLabel;
    QLabel *durationDetailLabel;
    QLabel *powerDetailLabel;

    QPushButton *startChargingBtn;
    QPushButton *settleOrderBtn;
    QPushButton *backHomeBtn;

    QTimer timer;

    void onStartCharging();
    void onSettleOrder();
    void resetToIdleState();
};

#endif
