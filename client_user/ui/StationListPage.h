#ifndef STATIONLISTPAGE_H
#define STATIONLISTPAGE_H

#include <QWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QCompleter>
#include <QStringListModel>
#include <QMap>

class StationListPage : public QWidget
{
    Q_OBJECT
public:
    explicit StationListPage(QWidget *parent = nullptr);
    void refreshStations();
    
    double getCurrentLat() const { return currentLat; }
    double getCurrentLng() const { return currentLng; }

signals:
    void stationSelected(int stationId);

private:
    QComboBox *regionCombo;
    QLineEdit *addressEdit;
    QPushButton *locateBtn;
    QTableWidget *stationTable;
    QNetworkAccessManager *networkManager;

    // 联想输入相关
    QCompleter *completer;
    QStringListModel *completerModel;
    // 用于保存建议名称与坐标 (lat, lng) 的映射
    QMap<QString, QPair<double, double>> suggestionCoords;

    double currentLat = 31.2304;
    double currentLng = 121.4737;

    void setupTable();
    void onLocate();
    void fetchAddressSuggestions(const QString &keyword);
    void geocodeAddress(const QString &address);
    void onStationClick();
};

#endif // STATIONLISTPAGE_H