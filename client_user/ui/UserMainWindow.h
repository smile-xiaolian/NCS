#ifndef USERMAINWINDOW_H
#define USERMAINWINDOW_H

#include <QWidget>
#include <QStackedWidget>
#include <QPushButton>

#include "LoginWindow.h"
#include "StationListPage.h"
#include "StationDetailPage.h"
#include "UserCenterPage.h"
#include "OrderListPage.h"
#include "SettlePage.h"
#include "MapWindow.h"
#include "core/models/User.h"

class UserMainWindow : public QWidget
{
    Q_OBJECT
public:
    explicit UserMainWindow(QWidget *parent = nullptr);

private:
    User currentUser;

    QStackedWidget *stackedWidget;
    QWidget *bottomNavBar;
    QPushButton *navHomeBtn;
    QPushButton *navOrdersBtn;
    QPushButton *navProfileBtn;

    LoginWindow *loginWindow;
    StationListPage *stationListPage;
    StationDetailPage *stationDetailPage;
    UserCenterPage *userCenterPage;
    OrderListPage *orderListPage;
    SettlePage *settlePage;
    MapWindow *mapWindow;

    void updateNavStyle(int activeIndex);
    bool checkUnsettledOrder();
};

#endif // USERMAINWINDOW_H