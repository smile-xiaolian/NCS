#include <QApplication>
#include <QWidget>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QTimer>

#include "ui/LoginPage.h"
#include "ui/HomePage.h"
#include "ui/StationDetailPage.h"
#include "ui/OrderPage.h"
#include "ui/ProfilePage.h"
#include "core/service/PlatformService.h"

class UserApp : public QWidget {
    QStackedWidget *p = new QStackedWidget;
    QPushButton *navHome = new QPushButton("🏠 首页");
    QPushButton *navOrders = new QPushButton("📋 我的订单");
    QPushButton *navProfile = new QPushButton("👤 个人中心");
    QWidget *bottomNavBar = new QWidget;

    LoginPage *loginPage = new LoginPage(this);
    HomePage *homePage = new HomePage(this);
    StationDetailPage *stationDetailPage = new StationDetailPage(this);
    ProfilePage *profilePage = new ProfilePage(this);
    OrderPage *orderPage = new OrderPage(this);

    User u;
    QTimer liveTimer;

    void updateNavStyle(int activeIndex) {
        QString activeStyle = "background-color: #2b4c7e; color: white; border-radius: 6px; font-weight: bold; padding: 8px;";
        QString normalStyle = "background-color: #f4f4f5; color: #606266; border-radius: 6px; padding: 8px;";
        navHome->setStyleSheet(activeIndex == 1 ? activeStyle : normalStyle);
        navOrders->setStyleSheet(activeIndex == 4 ? activeStyle : normalStyle);
        navProfile->setStyleSheet(activeIndex == 3 ? activeStyle : normalStyle);
    }

    bool checkUnsettledOrder(int &outOrderId) {
        auto ordersList = PlatformService::orders(u.id);
        for (auto &o : ordersList) {
            auto m = o.toMap();
            int stVal = m["status"].toInt();
            if (stVal == 0 || stVal == 1) {
                outOrderId = m["id"].toInt();
                return true;
            }
        }
        return false;
    }

public:
    UserApp() {
        setWindowTitle("NCS 电动汽车充电应用管理平台（用户端）");
        setFixedSize(420, 760);

        setStyleSheet(
            "QWidget { font-family: 'Microsoft YaHei', sans-serif; font-size: 13px; color: #2c3e50; }"
            "QLineEdit { padding: 8px 12px; border: 1px solid #dcdfe6; border-radius: 6px; background: #ffffff; }"
            "QPushButton { background-color: #409eff; color: white; border: none; border-radius: 6px; padding: 9px 16px; font-weight: bold; }"
            "QPushButton:hover { background-color: #66b1ff; }"
            "QPushButton#secondaryBtn { background-color: #f4f4f5; color: #606266; border: 1px solid #dcdfe6; }"
        );

        auto mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);
        mainLayout->addWidget(p);

        bottomNavBar->setFixedHeight(65);
        bottomNavBar->setStyleSheet("background-color: #ffffff; border-top: 1px solid #e4e7ed;");
        auto navLayout = new QHBoxLayout(bottomNavBar);
        navLayout->setContentsMargins(15, 8, 15, 8);
        navLayout->addWidget(navHome);
        navLayout->addWidget(navOrders);
        navLayout->addWidget(navProfile);
        mainLayout->addWidget(bottomNavBar);
        bottomNavBar->hide();

        p->addWidget(loginPage);       // 0
        p->addWidget(homePage);        // 1
        p->addWidget(stationDetailPage);// 2
        p->addWidget(profilePage);     // 3
        p->addWidget(orderPage);       // 4
        p->addWidget(orderPage->subStack); // 5 (结算/控制页复用)

        // 导航栏切换
        connect(navHome, &QPushButton::clicked, this, [this] { homePage->refreshStations(); p->setCurrentIndex(1); updateNavStyle(1); });
        connect(navOrders, &QPushButton::clicked, this, [this] { orderPage->refreshOrders(u.id); p->setCurrentIndex(4); updateNavStyle(4); });
        connect(navProfile, &QPushButton::clicked, this, [this] { profilePage->updateUserInfo(u); p->setCurrentIndex(3); updateNavStyle(3); });

        // 登录成功
        connect(loginPage, &LoginPage::loginSuccess, this, [this](const User &user) {
            u = user;
            homePage->refreshStations();
            p->setCurrentIndex(1);
            bottomNavBar->show();
            updateNavStyle(1);
        });

        // 首页点击电站 -> 检查未结算拦截 (UC-U-06)
        connect(homePage, &HomePage::requestStationDetail, this, [this](int stationId) {
            int dummyId = 0;
            if (checkUnsettledOrder(dummyId)) {
                QMessageBox::warning(this, "提示", "您有未完成的充电订单，请先结算");
                orderPage->subStack->setCurrentIndex(1);
                p->setCurrentIndex(5);
                return;
            }
            stationDetailPage->loadStation(stationId);
            p->setCurrentIndex(2);
        });

        // 选桩预约
        connect(stationDetailPage, &StationDetailPage::reserveSuccess, this, [this] {
            // 在详情页中获取当前选中的电桩
            // 简单处理：实际可通过信号把chargerId传过来
            // 此处触发预约逻辑
            p->setCurrentIndex(5);
            orderPage->subStack->setCurrentIndex(1);
        });
        connect(stationDetailPage, &StationDetailPage::requestBackHome, this, [this] {
            homePage->refreshStations(); p->setCurrentIndex(1); updateNavStyle(1);
        });

        // 订单页点击去结算
        connect(orderPage->orderTable, &QTableWidget::doubleClicked, this, [this] {
            auto item = orderPage->orderTable->currentItem();
            if (!item) return;
            int stVal = orderPage->orderTable->item(item->row(), 0)->data(Qt::UserRole + 1).toInt();
            if (stVal == 0 || stVal == 1) {
                orderPage->subStack->setCurrentIndex(1);
                p->setCurrentIndex(5);
            }
        });

        // 结算页动作
        connect(orderPage->startChargingBtn, &QPushButton::clicked, this, [this] {
            QString err;
            if (PlatformService::start(u.id, &err)) {
                QMessageBox::information(this, "提示", "已成功开始充电！");
            } else {
                QMessageBox::information(this, "提示", err.isEmpty() ? "启动失败" : err);
            }
        });

        connect(orderPage->settleOrderBtn, &QPushButton::clicked, this, [this] {
            QString err;
            if (PlatformService::settle(u.id, &err)) {
                QMessageBox::information(this, "提示", "结算完成！费用已从余额扣除。");
                u = PlatformService::loginOrRegister(u.phone);
                homePage->refreshStations();
                p->setCurrentIndex(1);
                updateNavStyle(1);
            } else {
                QMessageBox::information(this, "提示", err.isEmpty() ? "当前没有进行中的订单可结算" : err);
            }
        });

        // 个人中心退出登录
        connect(profilePage, &ProfilePage::logoutRequested, this, [this] {
            u = User();
            loginPage->clearInputs();
            bottomNavBar->hide();
            p->setCurrentIndex(0);
            QMessageBox::information(this, "提示", "已安全退出登录");
        });

        // 实时计费定时器
        liveTimer.start(1000);
        connect(&liveTimer, &QTimer::timeout, this, [this] {
            if (p->currentIndex() == 5) {
                orderPage->updateChargingStatus(u.id);
            }
        });
    }
};

int main(int c, char **v) {
    QApplication a(c, v);
    QString err;
    if (!PlatformService::initialize(&err)) {
        QMessageBox::critical(nullptr, "NCS 错误", err);
        return 1;
    }
    UserApp w;
    w.show();
    return a.exec();
}