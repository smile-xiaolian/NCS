#ifndef STATIONDETAILPAGE_H
#define STATIONDETAILPAGE_H

#include <QWidget>
#include <QLabel>
#include <QTableWidget>
#include <QPushButton>

class StationDetailPage : public QWidget
{
    Q_OBJECT
public:
    explicit StationDetailPage(QWidget *parent = nullptr);
    void loadStation(int stationId, int userId);

signals:
    void backToHomeRequested();
    void reservationSuccess();
    void navigateRequested(double lat, double lng, const QString &stationName);

private:
    int currentStationId = 0;
    int currentUserId = 0;

    QLabel *titleLabel;
    QTableWidget *chargerTable;
    QPushButton *reserveBtn;
    QPushButton *navigateBtn;
    QPushButton *backBtn;

    void setupTable();
    void onReserve();
    void onNavigate();
};

#endif // STATIONDETAILPAGE_H