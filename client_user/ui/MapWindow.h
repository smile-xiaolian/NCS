#ifndef MAPWINDOW_H
#define MAPWINDOW_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QWebEngineView>

class MapWindow : public QWidget
{
    Q_OBJECT
public:
    explicit MapWindow(QWidget *parent = nullptr);
    void loadRoute(double startLat, double startLng, double endLat, double endLng, const QString &stationName);

signals:
    void backRequested();

private:
    // 出行方式选择
    QComboBox *modeCombo;
    
    // 起终点信息展示
    QLabel *startCoordLabel;
    QLabel *endCoordLabel;
    QLabel *distanceLabel;
    QLabel *stationNameLabel;

    QPushButton *openBrowserBtn;
    QPushButton *backBtn;

    // 内嵌 WebEngine 视图
    QWebEngineView *webView;
    bool m_isPageLoaded = false; // 记录 WebEngine 页面是否真正加载完毕

    // 存储当前导航点位数据
    double m_startLat = 31.2304;
    double m_startLng = 121.4737;
    double m_endLat = 0.0;
    double m_endLng = 0.0;
    QString m_stationName;

    void updateMapRoute();
    void openTencentMapUrl();
};

#endif // MAPWINDOW_H