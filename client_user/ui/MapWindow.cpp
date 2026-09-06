#include "MapWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QDesktopServices>
#include <QUrl>
#include "core/utils/Haversine.h"

MapWindow::MapWindow(QWidget *parent) : QWidget(parent)
{
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(15, 15, 15, 15);
    layout->setSpacing(15);

    // 1. 顶部栏：返回按钮与标题
    auto topLayout = new QHBoxLayout();
    backBtn = new QPushButton("返回", this);
    backBtn->setObjectName("secondaryBtn");
    backBtn->setMaximumWidth(80);

    auto titleLabel = new QLabel("一键路线导航 (方案 B - 保底方案)", this);
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
    modeCombo->addItem("🚗 驾车路线", "drive");
    modeCombo->addItem("🚶 步行路线", "walk");
    modeCombo->addItem("🚌 公交路线", "bus");

    modeLayout->addWidget(modeTitle);
    modeLayout->addWidget(modeCombo, 1);
    layout->addLayout(modeLayout);

    // 3. 页面主体：路线与坐标卡片区域
    auto cardFrame = new QFrame(this);
    cardFrame->setStyleSheet(
        "QFrame { background-color: #ffffff; border-radius: 10px; border: 1px solid #dcdfe6; }"
    );
    auto cardLayout = new QVBoxLayout(cardFrame);
    cardLayout->setContentsMargins(20, 20, 20, 20);
    cardLayout->setSpacing(12);

    stationNameLabel = new QLabel("目标电站：-", this);
    stationNameLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #409eff; border: none;");
    
    startCoordLabel = new QLabel("起点坐标 (当前定位)：-", this);
    startCoordLabel->setStyleSheet("color: #606266; font-size: 13px; border: none;");

    endCoordLabel = new QLabel("终点坐标 (目标电站)：-", this);
    endCoordLabel->setStyleSheet("color: #606266; font-size: 13px; border: none;");

    distanceLabel = new QLabel("测算直线距离：- km", this);
    distanceLabel->setStyleSheet("color: #e6a23c; font-size: 14px; font-weight: bold; border: none;");

    cardLayout->addWidget(stationNameLabel);
    cardLayout->addWidget(new QLabel("<hr style='border: none; border-top: 1px solid #f0f0f0;'>", this));
    cardLayout->addWidget(startCoordLabel);
    cardLayout->addWidget(endCoordLabel);
    cardLayout->addWidget(distanceLabel);
    cardLayout->addStretch();

    layout->addWidget(cardFrame, 1);

    // 4. 调起系统浏览器导航按钮
    openBrowserBtn = new QPushButton("🌐 在系统浏览器中打开腾讯地图路线规划", this);
    openBrowserBtn->setStyleSheet(
        "QPushButton { background-color: #67c23a; color: white; border-radius: 6px; padding: 12px; font-weight: bold; font-size: 14px; }"
        "QPushButton:hover { background-color: #85ce61; }"
        "QPushButton:pressed { background-color: #5daf34; }"
    );
    layout->addWidget(openBrowserBtn);

    // 5. 信号槽关联
    connect(backBtn, &QPushButton::clicked, this, &MapWindow::backRequested);
    connect(openBrowserBtn, &QPushButton::clicked, this, &MapWindow::openTencentMapUrl);
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
}

void MapWindow::openTencentMapUrl()
{
    QString mode = modeCombo->currentData().toString();
    // 使用 QDesktopServices::openUrl 调起腾讯地图路线规划网页
    QString urlStr = QString("https://apis.map.qq.com/uri/v1/routeplan?type=%1&to=%2&coord=%3,%4&policy=0&referer=ncs")
                        .arg(mode)
                        .arg(m_stationName)
                        .arg(m_endLat)
                        .arg(m_endLng);

    QDesktopServices::openUrl(QUrl(urlStr));
}