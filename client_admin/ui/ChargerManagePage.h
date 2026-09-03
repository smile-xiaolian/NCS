#pragma once

#include <QWidget>

class QComboBox;
class QTableWidget;

class ChargerManagePage : public QWidget
{
public:
    explicit ChargerManagePage(QWidget *parent = nullptr);
    void refresh();

private:
    void reloadStationFilter();
    void fillTable();
    QVariantMap selectedCharger() const;
    void showNoSelection() const;

    void addCharger();
    void addChargersInBatch();
    void editCharger();
    void removeCharger();
    void restartCharger();

    QComboBox *mStationFilter = nullptr;
    QComboBox *mStatusFilter = nullptr;
    QTableWidget *mTable = nullptr;
};