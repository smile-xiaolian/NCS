#include "UserMainWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include "core/service/PlatformService.h"

UserMainWindow::UserMainWindow(QWidget *parent) : QWidget(parent)
{
    setWindowTitle("NCS 电动汽车充电应用管理平台（用户端）");
    setFixedSize(420, 760);

    setStyleSheet(
        "QWidget { font-family: 'Microsoft YaHei', sans-serif; font-size: 13px; color: #2c3e50; }"
        "QLineEdit { padding: 8px 12px; border: 1px solid #dcdfe6; border-radius: 6px; background: #ffffff; }"
        "QLineEdit:focus { border-color: #409eff; }"
        "QPushButton { background-color: #409eff; color: white; border: none; border-radius: 6px; padding: 9px 16px; font-weight: bold; }"
        "QPushButton:hover { background-color: #66b1ff; }"
        "QPushButton:pressed { background-color: #3a8ee6; }"
        "QPushButton#secondaryBtn { background-color: #f4f4f5; color: #606266; border: 1px solid #dcdfe6; }"
        "QPushButton#secondaryBtn:hover { background-color: #e4e7ed; }"
    );

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    stackedWidget = new QStackedWidget;
    mainLayout->addWidget(stackedWidget);

    // 底部导航栏
    bottomNavBar = new QWidget;
    bottomNavBar->setFixedHeight(65);
    bottomNavBar->setStyleSheet("background-color: #ffffff; border-top: 1px solid #e4e7ed;");
    auto navLayout = new QHBoxLayout(bottomNavBar);
    navLayout->setContentsMargins(15, 8, 15, 8);
    navLayout->setSpacing(10);

    navHomeBtn = new QPushButton("首页");
    navOrdersBtn = new QPushButton("我的订单");
    navProfileBtn = new QPushButton("个人中心");

    navLayout->addWidget(navHomeBtn);
    navLayout->addWidget(navOrdersBtn);
    navLayout->addWidget(navProfileBtn);
    mainLayout->addWidget(bottomNavBar);
    bottomNavBar->hide();

    // 实例化页面
    loginWindow = new LoginWindow;
    stationListPage = new StationListPage;
    stationDetailPage = new StationDetailPage;
    userCenterPage = new UserCenterPage;
    orderListPage = new OrderListPage;
    settlePage = new SettlePage;
    mapWindow = new MapWindow;

    stackedWidget->addWidget(loginWindow);        // 0
    stackedWidget->addWidget(stationListPage);     // 1
    stackedWidget->addWidget(stationDetailPage);   // 2
    stackedWidget->addWidget(userCenterPage);      // 3
    stackedWidget->addWidget(orderListPage);       // 4
    stackedWidget->addWidget(settlePage);          // 5
    stackedWidget->addWidget(mapWindow);           // 6

    // 绑定信号槽
    connect(loginWindow, &LoginWindow::loginSuccess, this, [this](const User &u) {
        currentUser = u;
        bottomNavBar->show();
        stationListPage->refreshStations();
        stackedWidget->setCurrentIndex(1);
        updateNavStyle(1);
    });

    connect(navHomeBtn, &QPushButton::clicked, this, [this] {
        stationListPage->refreshStations();
        stackedWidget->setCurrentIndex(1);
        updateNavStyle(1);
    });

    connect(navOrdersBtn, &QPushButton::clicked, this, [this] {
        orderListPage->loadOrders(currentUser.id);
        stackedWidget->setCurrentIndex(4);
        updateNavStyle(4);
    });

    connect(navProfileBtn, &QPushButton::clicked, this, [this] {
        userCenterPage->setUser(currentUser);
        stackedWidget->setCurrentIndex(3);
        updateNavStyle(3);
    });

    connect(stationListPage, &StationListPage::stationSelected, this, [this](int stationId) {
        if (checkUnsettledOrder()) {
            QMessageBox::warning(this, "提示", "您有未完成的充电订单，请先结算");
            settlePage->setUserId(currentUser.id);
            stackedWidget->setCurrentIndex(5);
            return;
        }
        stationDetailPage->loadStation(stationId, currentUser.id);
        stackedWidget->setCurrentIndex(2);
    });

    connect(stationDetailPage, &StationDetailPage::backToHomeRequested, this, [this] {
        stationListPage->refreshStations();
        stackedWidget->setCurrentIndex(1);
        updateNavStyle(1);
    });

    connect(stationDetailPage, &StationDetailPage::navigateRequested, this, [this](double lat, double lng, const QString &name) {
        // 起点为人民广场当前定位，终点为所选电站坐标
        mapWindow->loadRoute(31.2304, 121.4737, lat, lng, name);
        stackedWidget->setCurrentIndex(6); // 切换至导航页
    });

    connect(mapWindow, &MapWindow::backRequested, this, [this] {
        stackedWidget->setCurrentIndex(2); // 返回详情页
    });

    connect(stationDetailPage, &StationDetailPage::reservationSuccess, this, [this] {
        settlePage->setUserId(currentUser.id);
        stackedWidget->setCurrentIndex(5);
    });

    connect(userCenterPage, &UserCenterPage::logoutRequested, this, [this] {
        currentUser = User();
        bottomNavBar->hide();
        stackedWidget->setCurrentIndex(0);
        QMessageBox::information(this, "提示", "已安全退出登录");
    });

    connect(userCenterPage, &UserCenterPage::userUpdated, this, [this](const User &updated) {
        currentUser = updated;
    });

    connect(orderListPage, &OrderListPage::goToSettleRequested, this, [this] {
        settlePage->setUserId(currentUser.id);
        stackedWidget->setCurrentIndex(5);
    });

    connect(settlePage, &SettlePage::backToHomeRequested, this, [this] {
        stationListPage->refreshStations();
        stackedWidget->setCurrentIndex(1);
        updateNavStyle(1);
    });

    connect(settlePage, &SettlePage::settleSuccess, this, [this] {
        currentUser = PlatformService::loginOrRegister(currentUser.phone);
        stationListPage->refreshStations();
        stackedWidget->setCurrentIndex(1);
        updateNavStyle(1);
    });
}

void UserMainWindow::updateNavStyle(int activeIndex)
{
    QString activeStyle = "background-color: #2b4c7e; color: white; border-radius: 6px; font-weight: bold; padding: 8px;";
    QString normalStyle = "background-color: #f4f4f5; color: #606266; border-radius: 6px; padding: 8px;";

    navHomeBtn->setStyleSheet(activeIndex == 1 ? activeStyle : normalStyle);
    navOrdersBtn->setStyleSheet(activeIndex == 4 ? activeStyle : normalStyle);
    navProfileBtn->setStyleSheet(activeIndex == 3 ? activeStyle : normalStyle);
}

bool UserMainWindow::checkUnsettledOrder()
{
    auto ordersList = PlatformService::orders(currentUser.id);
    for (auto &o : ordersList) {
        int stVal = o.toMap()["status"].toInt();
        if (stVal == 0 || stVal == 1) {
            return true;
        }
    }
    return false;
}