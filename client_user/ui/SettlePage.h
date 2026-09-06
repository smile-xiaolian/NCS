#ifndef SETTLEPAGE_H
#define SETTLEPAGE_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QTimer>

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
    QLabel *chargingStatusLabel;
    QPushButton *startChargingBtn;
    QPushButton *settleOrderBtn;
    QPushButton *backHomeBtn;
    QTimer timer;

    void onStartCharging();
    void onSettleOrder();
};

#endif // SETTLEPAGE_H