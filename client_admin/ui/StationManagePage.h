#pragma once

#include <QWidget>

class QLabel;
class QTableWidget;

class StationManagePage : public QWidget
{
public:
    explicit StationManagePage(QWidget *parent = nullptr);
    void refresh();

private:
    void fillStationTable();
    void updateChargerDetail(int stationRow);
    QVariantMap selectedStation() const;
    void showNoSelection() const;

    void addStation();
    void editStation();
    void removeStation();

    QTableWidget *mStationTable = nullptr;
    QTableWidget *mChargerTable = nullptr;
    QLabel *mDetailTitle = nullptr;
};