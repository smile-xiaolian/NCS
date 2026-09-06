#ifndef STATIONLISTPAGE_H
#define STATIONLISTPAGE_H

#include <QWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>

class StationListPage : public QWidget
{
    Q_OBJECT
public:
    explicit StationListPage(QWidget *parent = nullptr);
    void refreshStations();

signals:
    void stationSelected(int stationId);

private:
    QComboBox *regionCombo;
    QLineEdit *addressEdit;
    QPushButton *locateBtn;
    QTableWidget *stationTable;

    double currentLat = 31.2304;
    double currentLng = 121.4737;

    void setupTable();
    void onLocate();
    void onStationClick();
};

#endif // STATIONLISTPAGE_H