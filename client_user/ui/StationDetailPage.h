#ifndef STATIONDETAILPAGE_H
#define STATIONDETAILPAGE_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QEvent>

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

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    int currentStationId = 0;
    int currentUserId = 0;

    QLabel *titleLabel;

    // 替代原有的 QTableWidget
    QScrollArea *scrollArea;
    QWidget *cardContainerWidget;
    QVBoxLayout *cardContainerLayout;

    QPushButton *navigateBtn;
    QPushButton *backBtn;

    QWidget* createChargerCard(const QVariantMap &chargerMap);
    void reserveCharger(int chargerId);
    void onNavigate();
};

#endif // STATIONDETAILPAGE_H
