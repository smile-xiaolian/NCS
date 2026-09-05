#pragma once
#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>

class OrderPage : public QWidget {
    Q_OBJECT
public:
    OrderPage(QWidget *parent = nullptr);
    void refreshOrders(int userId);
    void updateChargingStatus(int userId);

signals:
    void requestBackHome();
    void settlementFinished();

public:
    QStackedWidget *subStack;
    QTableWidget *orderTable;
    QLabel *chargingStatusLabel;
    QPushButton *startChargingBtn;
    QPushButton *settleOrderBtn;
};