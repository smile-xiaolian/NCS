#pragma once
#include <QWidget>
#include <QLabel>
#include <QTableWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>

class HomePage : public QWidget {
    Q_OBJECT
public:
    HomePage(QWidget *parent = nullptr);
    void refreshStations();

signals:
    void requestStationDetail(int stationId);

private:
    QComboBox *regionCombo;
    QLineEdit *addressEdit;
    QPushButton *locateBtn;
    QTableWidget *stationTable;
    QPushButton *enterDetailBtn;
    
    double currentLat = 31.2304;
    double currentLng = 121.4737;
};