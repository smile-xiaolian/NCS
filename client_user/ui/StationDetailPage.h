#pragma once
#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>

class StationDetailPage : public QWidget {
    Q_OBJECT
public:
    StationDetailPage(QWidget *parent = nullptr);
    void loadStation(int stationId);

signals:
    void reserveSuccess();
    void requestBackHome();

private:
    QLabel *stationTitleLabel;
    QTableWidget *chargerTable;
    QPushButton *reserveBtn;
    QPushButton *navigateBtn;
    QPushButton *backHomeBtn;
    int currentStationId = 0;
};