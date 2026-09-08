#include "MapWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QDesktopServices>
#include <QUrl>
#include <QFile>
#include <QDebug>
#include "core/utils/Haversine.h"

MapWindow::MapWindow(QWidget *parent) : QWidget(parent)
{
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(15, 15, 15, 15);
    layout->setSpacing(10);

    // 1. 顶部栏：返回按钮与标题
    auto topLayout = new QHBoxLayout();
    backBtn = new QPushButton("返回", this);
    backBtn->setObjectName("secondaryBtn");
    backBtn->setMaximumWidth(80);

    auto titleLabel = new QLabel("一键路线导航", this);
    titleLabel->setStyleSheet("font-size: 15px; font-weight: bold; color: #2b4c7e;");

    topLayout->addWidget(backBtn);
    topLayout->addSpacing(10);
    topLayout->addWidget(titleLabel);
    topLayout->addStretch();
    layout->addLayout(topLayout);

    // 2. 出行方式选择区
    auto modeLayout = new QHBoxLayout();
    auto modeTitle = new QLabel("<b>选择出行方式：</b>", this);
    modeCombo = new QComboBox(this);
    modeCombo->addItem("驾车路线", "drive");
    modeCombo->addItem("步行路线", "walk");
    modeCombo->addItem("公交路线", "bus");

    modeLayout->addWidget(modeTitle);
    modeLayout->addWidget(modeCombo, 1);
    layout->addLayout(modeLayout);

    // 3. 内嵌 QWebEngineView 腾讯地图视图
    webView = new QWebEngineView(this);
m_isPageLoaded = false;

// 设置自定义 WebPage 捕获控制台日志[cite: 22]
webView->setPage(new CustomWebPage(webView));

// 监听 HTML 加载状态[cite: 22]
connect(webView, &QWebEngineView::loadFinished, this, [this](bool ok) {
    if (ok) {
        m_isPageLoaded = true;
        // 页面加载成功后再更新路线[cite: 22]
        updateMapRoute();
    } else {
        qWarning() << "错误：地图 HTML 页面加载失败！";
    }
});

// 使用 QFile 读取内嵌的资源文件文本
QFile htmlFile(":/resources/map_template.html");
if (htmlFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    QString htmlContent = QString::fromUtf8(htmlFile.readAll());
    htmlFile.close();
    
    // 使用 loadHtml 并指定 Base URL 为 https，确保腾讯地图 JS SDK 能正常访问外部网络
    webView->setHtml(htmlContent, QUrl("https://map.qq.com"));
} else {
    qWarning() << "无法读取资源文件 :/resources/map_template.html";
}

webView->setMinimumHeight(350);
layout->addWidget(webView, 2);

    // 4. 路线与坐标卡片区域
    auto cardFrame = new QFrame(this);
    cardFrame->setStyleSheet(
        "QFrame { background-color: #ffffff; border-radius: 10px; border: 1px solid #dcdfe6; }"
    );
    auto cardLayout = new QVBoxLayout(cardFrame);
    cardLayout->setContentsMargins(15, 15, 15, 15);
    cardLayout->setSpacing(8);

    stationNameLabel = new QLabel("目标电站：-", this);
    stationNameLabel->setStyleSheet("font-size: 15px; font-weight: bold; color: #409eff; border: none;");
    
    startCoordLabel = new QLabel("起点坐标 (当前定位)：-", this);
    startCoordLabel->setStyleSheet("color: #606266; font-size: 12px; border: none;");

    endCoordLabel = new QLabel("终点坐标 (目标电站)：-", this);
    endCoordLabel->setStyleSheet("color: #606266; font-size: 12px; border: none;");

    distanceLabel = new QLabel("测算直线距离：- km", this);
    distanceLabel->setStyleSheet("color: #e6a23c; font-size: 13px; font-weight: bold; border: none;");

    cardLayout->addWidget(stationNameLabel);
    cardLayout->addWidget(startCoordLabel);
    cardLayout->addWidget(endCoordLabel);
    cardLayout->addWidget(distanceLabel);

    layout->addWidget(cardFrame);

    // 5. 调起系统浏览器导航按钮
    openBrowserBtn = new QPushButton("在系统浏览器中打开腾讯地图路线规划", this);
    openBrowserBtn->setStyleSheet(
        "QPushButton { background-color: #67c23a; color: white; border-radius: 6px; padding: 10px; font-weight: bold; font-size: 13px; }"
        "QPushButton:hover { background-color: #85ce61; }"
        "QPushButton:pressed { background-color: #5daf34; }"
    );
    layout->addWidget(openBrowserBtn);

    // 6. 信号槽关联
    connect(backBtn, &QPushButton::clicked, this, &MapWindow::backRequested);
    connect(openBrowserBtn, &QPushButton::clicked, this, &MapWindow::openTencentMapUrl);
    connect(modeCombo, &QComboBox::currentIndexChanged, this, &MapWindow::updateMapRoute);
}

void MapWindow::loadRoute(double startLat, double startLng, double endLat, double endLng, const QString &stationName)
{
    m_startLat = startLat;
    m_startLng = startLng;
    m_endLat = endLat;
    m_endLng = endLng;
    m_stationName = stationName;

    // 计算直线距离
    double distKm = haversineKm(startLat, startLng, endLat, endLng);

    stationNameLabel->setText(QString("目标电站：%1").arg(stationName));
    startCoordLabel->setText(QString("起点坐标 (当前定位)：%1, %2").arg(startLat, 0, 'f', 4).arg(startLng, 0, 'f', 4));
    endCoordLabel->setText(QString("终点坐标 (目标电站)：%1, %2").arg(endLat, 0, 'f', 4).arg(endLng, 0, 'f', 4));
    distanceLabel->setText(QString("测算直线距离：约 %1 km").arg(distKm, 0, 'f', 1));

    updateMapRoute();
}

void MapWindow::updateMapRoute()
{
    // 如果 HTML 页面还未载入完毕，直接拦截，避免 Chromium IPC 校验失败闪退
    if (!m_isPageLoaded) {
        return;
    }

    QString mode = modeCombo->currentData().toString();
    QString jsCode = QString("renderRoute(%1, %2, %3, %4, '%5');")
                        .arg(m_startLat, 0, 'f', 6)
                        .arg(m_startLng, 0, 'f', 6)
                        .arg(m_endLat, 0, 'f', 6)
                        .arg(m_endLng, 0, 'f', 6)
                        .arg(mode);

    // 页面安全载入后执行 JavaScript
    webView->page()->runJavaScript(jsCode);
}

void MapWindow::openTencentMapUrl()
{
    QString mode = modeCombo->currentData().toString();
    QString urlStr = QString("https://apis.map.qq.com/uri/v1/routeplan?type=%1&to=%2&coord=%3,%4&policy=0&referer=ncs")
                        .arg(mode)
                        .arg(m_stationName)
                        .arg(m_endLat)
                        .arg(m_endLng);

    QDesktopServices::openUrl(QUrl(urlStr));
}
