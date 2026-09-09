#pragma once

#include <QMainWindow>
#include <QString>

class QLineEdit;
class QListWidget;
class QLabel;
class QStackedWidget;
class ChargerManagePage;
class ChargerStatusPage;
class RevenuePage;
class StationManagePage;
class UserManagePage;

class AdminMainWindow : public QMainWindow
{
public:
    explicit AdminMainWindow(QWidget *parent = nullptr);

private:
    void buildLoginPage();
    void buildWorkspace();
    void tryLogin();
    void refreshCurrentPage();
    void refreshAll();
    void updateStatusBar();

    QStackedWidget *m_stack = nullptr;
    QWidget *m_loginPage = nullptr;
    QWidget *m_workspace = nullptr;
    QListWidget *m_nav = nullptr;
    QStackedWidget *m_pages = nullptr;
    QLineEdit *m_account = nullptr;
    QLineEdit *m_password = nullptr;
    QLabel *m_accountName = nullptr;
    QString m_adminAccount;

    RevenuePage *m_revenuePage = nullptr;
    ChargerStatusPage *m_statusPage = nullptr;
    ChargerManagePage *m_chargerPage = nullptr;
    StationManagePage *m_stationPage = nullptr;
    UserManagePage *m_userPage = nullptr;
};
