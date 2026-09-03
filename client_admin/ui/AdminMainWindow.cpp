#include "AdminMainWindow.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QStatusBar>
#include <QVBoxLayout>

#include "core/service/PlatformService.h"
#include "ChargerManagePage.h"
#include "ChargerStatusPage.h"
#include "FormatUtil.h"
#include "RevenuePage.h"
#include "StationManagePage.h"
#include "UserManagePage.h"

AdminMainWindow::AdminMainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("NCS 运营管理后台"));
    resize(1280, 800);

    m_stack = new QStackedWidget(this);
    setCentralWidget(m_stack);

    buildLoginPage();
    buildWorkspace();

    statusBar()->addPermanentWidget(
        new QLabel(QStringLiteral("数据库：") + PlatformService::databasePath()));
    m_stack->setCurrentWidget(m_loginPage);
}

void AdminMainWindow::buildLoginPage()
{
    m_loginPage = new QWidget;
    auto *root = new QVBoxLayout(m_loginPage);
    root->addStretch(2);

    auto *panel = new QWidget;
    panel->setFixedWidth(380);
    auto *form = new QVBoxLayout(panel);

    auto *title = new QLabel(QStringLiteral("<h2 style='color:#1f6feb;'>NCS 运营管理后台</h2>"));
    title->setAlignment(Qt::AlignCenter);
    auto *subtitle = new QLabel(QStringLiteral("管理员登录"));
    subtitle->setAlignment(Qt::AlignCenter);
    subtitle->setStyleSheet(QStringLiteral("color:#8b949e;"));

    m_account = new QLineEdit;
    m_account->setPlaceholderText(QStringLiteral("管理员账号"));
    m_password = new QLineEdit;
    m_password->setPlaceholderText(QStringLiteral("密码"));
    m_password->setEchoMode(QLineEdit::Password);

    auto *loginButton = new QPushButton(QStringLiteral("登 录"));
    loginButton->setDefault(true);

    auto *hint = new QLabel(QStringLiteral("默认账号 admin / 密码 123456"));
    hint->setAlignment(Qt::AlignCenter);
    hint->setStyleSheet(QStringLiteral("color:#8b949e;font-size:12px;"));

    form->addWidget(title);
    form->addWidget(subtitle);
    form->addSpacing(16);
    form->addWidget(m_account);
    form->addSpacing(8);
    form->addWidget(m_password);
    form->addSpacing(16);
    form->addWidget(loginButton);
    form->addSpacing(10);
    form->addWidget(hint);

    auto *centerRow = new QHBoxLayout;
    centerRow->addStretch(1);
    centerRow->addWidget(panel);
    centerRow->addStretch(1);
    root->addLayout(centerRow);
    root->addStretch(2);

    connect(m_account, &QLineEdit::returnPressed, this, [this] { tryLogin(); });
    connect(m_password, &QLineEdit::returnPressed, this, [this] { tryLogin(); });
    connect(loginButton, &QPushButton::clicked, this, [this] { tryLogin(); });

    m_stack->addWidget(m_loginPage);
}

void AdminMainWindow::buildWorkspace()
{
    m_workspace = new QWidget;
    auto *root = new QHBoxLayout(m_workspace);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    m_nav = new QListWidget;
    m_nav->setFixedWidth(190);
    m_nav->addItems({QStringLiteral("营收分析"),
                     QStringLiteral("电桩状态总览"),
                     QStringLiteral("充电桩管理"),
                     QStringLiteral("充电站管理"),
                     QStringLiteral("用户管理")});

    m_pages = new QStackedWidget;
    m_revenuePage = new RevenuePage;
    m_statusPage = new ChargerStatusPage;
    m_chargerPage = new ChargerManagePage;
    m_stationPage = new StationManagePage;
    m_userPage = new UserManagePage;
    m_pages->addWidget(m_revenuePage);
    m_pages->addWidget(m_statusPage);
    m_pages->addWidget(m_chargerPage);
    m_pages->addWidget(m_stationPage);
    m_pages->addWidget(m_userPage);

    root->addWidget(m_nav);
    root->addWidget(m_pages, 1);

    connect(m_nav, &QListWidget::currentRowChanged, this, [this](int row) {
        if (row < 0) {
            return;
        }
        m_pages->setCurrentIndex(row);
        refreshCurrentPage();
    });

    m_stack->addWidget(m_workspace);
    m_nav->setCurrentRow(0);
}

void AdminMainWindow::tryLogin()
{
    const QString account = m_account->text().trimmed();
    const QString password = m_password->text();
    if (account.isEmpty() || password.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("提示"),
                                 QStringLiteral("请输入账号和密码"));
        return;
    }
    if (!PlatformService::adminLogin(account, password)) {
        QMessageBox::warning(this, QStringLiteral("登录失败"),
                             QStringLiteral("账号或密码错误，请重新输入"));
        m_password->selectAll();
        m_password->setFocus();
        return;
    }

    m_adminAccount = account;
    m_password->clear();
    refreshAll();
    m_stack->setCurrentWidget(m_workspace);
}

void AdminMainWindow::refreshCurrentPage()
{
    switch (m_nav->currentRow()) {
    case 0: m_revenuePage->refresh(); break;
    case 1: m_statusPage->refresh(); break;
    case 2: m_chargerPage->refresh(); break;
    case 3: m_stationPage->refresh(); break;
    case 4: m_userPage->refresh(); break;
    default: break;
    }
}

void AdminMainWindow::refreshAll()
{
    m_revenuePage->refresh();
    m_statusPage->refresh();
    m_chargerPage->refresh();
    m_stationPage->refresh();
    m_userPage->refresh();
    updateStatusBar();
}

void AdminMainWindow::updateStatusBar()
{
    const QVariantMap overview = PlatformService::metrics();
    const QVariantMap chargers = PlatformService::chargerOverview();
    statusBar()->showMessage(
        QStringLiteral("登录账号：%1    完成订单 %2 单 · 总营收 ¥%3 · 在线电桩 %4/%5 · 注册用户 %6 人")
            .arg(m_adminAccount)
            .arg(overview.value(QStringLiteral("orders")).toInt())
            .arg(ncs::number(overview.value(QStringLiteral("revenue")).toDouble()))
            .arg(overview.value(QStringLiteral("online")).toInt())
            .arg(chargers.value(QStringLiteral("total")).toInt())
            .arg(overview.value(QStringLiteral("users")).toInt()));
}