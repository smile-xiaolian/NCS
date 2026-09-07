#ifndef STATIONLISTPAGE_H
#define STATIONLISTPAGE_H

#include <QWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QCompleter>
#include <QStringListModel>
#include <QMap>
#include <QEvent> // 

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
    
protected:
    // <--- 补充 eventFilter 的虚函数重写声明
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QComboBox *regionCombo;
    QLineEdit *addressEdit;
    QPushButton *locateBtn;

    // 替换原有的 QTableWidget，使用滚动区域放置小卡片列表
    QScrollArea *scrollArea;
    QWidget *cardContainerWidget;
    QVBoxLayout *cardContainerLayout;

    QNetworkAccessManager *networkManager;

    // 联想输入相关
    QCompleter *completer;
    QStringListModel *completerModel;
    QMap<QString, QPair<double, double>> suggestionCoords;

    double currentLat = 31.2304;
    double currentLng = 121.4737;

    void onLocate();
    void fetchAddressSuggestions(const QString &keyword);
    void geocodeAddress(const QString &address);
    QWidget* createStationCard(const QVariantMap &stationMap);
};

#endif // STATIONLISTPAGE_H
