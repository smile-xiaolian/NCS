#include "AdminMainWindow.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QFrame>
#include <QStackedWidget>
#include <QStatusBar>
#include <QVBoxLayout>

#include "core/service/PlatformService.h"
#include "ChargerManagePage.h"
#include "ChargerStatusPage.h"
#include "FormatUtil.h"
#include "RevenuePage.h"
#include "StationManagePage.h"
#include "UiKit.h"
#include "UserManagePage.h"

AdminMainWindow::AdminMainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("NCS 运营管理后台"));
    resize(1360, 860);
    setMinimumSize(1200, 720);

    m_stack = new QStackedWidget(this);
    setCentralWidget(m_stack);

    buildLoginPage();
    buildWorkspace();
    m_stack->setCurrentWidget(m_loginPage);
}

void AdminMainWindow::buildLoginPage()
{
    m_loginPage = new QWidget;
    m_loginPage->setObjectName(QStringLiteral("loginPage"));
    auto *root = new QVBoxLayout(m_loginPage);
    root->addStretch(2);

    auto *panel = new QFrame;
    panel->setObjectName(QStringLiteral("loginCard"));
    panel->setFixedWidth(400);
    auto *form = new QVBoxLayout(panel);
    form->setContentsMargins(40, 36, 40, 30);
    form->setSpacing(8);

    auto *brandRow = new QHBoxLayout;
    brandRow->setSpacing(12);
    auto *logo = new QLabel(QStringLiteral("N"), panel);
    logo->setObjectName(QStringLiteral("brandLogo"));
    logo->setFixedSize(44, 44);
    logo->setAlignment(Qt::AlignCenter);
    brandRow->addStretch(1);
    brandRow->addWidget(logo);
    brandRow->addStretch(1);
    form->addLayout(brandRow);
    form->addSpacing(12);

    auto *title = new QLabel(QStringLiteral("NCS 运营管理后台"), panel);
    title->setObjectName(QStringLiteral("loginTitle"));
    title->setAlignment(Qt::AlignCenter);
    auto *subtitle = new QLabel(QStringLiteral("管理员登录 · 充电网络运营管理系统"), panel);
    subtitle->setObjectName(QStringLiteral("loginSub"));
    subtitle->setAlignment(Qt::AlignCenter);

    m_account = new QLineEdit(panel);
    m_account->setPlaceholderText(QStringLiteral("管理员账号"));
    m_password = new QLineEdit(panel);
    m_password->setPlaceholderText(QStringLiteral("密码"));
    m_password->setEchoMode(QLineEdit::Password);

    auto *loginButton = new QPushButton(QStringLiteral("登 录"), panel);
    loginButton->setObjectName(QStringLiteral("loginButton"));
    loginButton->setCursor(Qt::PointingHandCursor);
    loginButton->setDefault(true);

    auto *divider = new QFrame(panel);
    divider->setObjectName(QStringLiteral("divider"));
    divider->setFixedHeight(1);
    auto *hint = new QLabel(QStringLiteral("默认账号 admin / 密码 123456"), panel);
    hint->setObjectName(QStringLiteral("loginHint"));
    hint->setAlignment(Qt::AlignCenter);

    form->addWidget(title);
    form->addSpacing(2);
    form->addWidget(subtitle);
    form->addSpacing(22);
    form->addWidget(m_account);
    form->addSpacing(12);
    form->addWidget(m_password);
    form->addSpacing(18);
    form->addWidget(loginButton);
    form->addSpacing(16);
    form->addWidget(divider);
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
    m_workspace->setObjectName(QStringLiteral("workspace"));
    auto *root = new QHBoxLayout(m_workspace);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // 侧边栏:品牌区 + 导航 + 账号卡片
    auto *sidebar = new QFrame(m_workspace);
    sidebar->setObjectName(QStringLiteral("sidebar"));
    sidebar->setFixedWidth(212);
    auto *sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(14, 18, 14, 16);
    sidebarLayout->setSpacing(0);

    auto *brandRow = new QHBoxLayout;
    brandRow->setSpacing(10);
    auto *logo = new QLabel(QStringLiteral("N"), sidebar);
    logo->setObjectName(QStringLiteral("brandLogo"));
    logo->setFixedSize(38, 38);
    logo->setAlignment(Qt::AlignCenter);
    brandRow->addWidget(logo);
    auto *brandText = new QVBoxLayout;
    brandText->setSpacing(1);
    auto *brandTitle = new QLabel(QStringLiteral("NCS 运营平台"), sidebar);
    brandTitle->setObjectName(QStringLiteral("brandTitle"));
    auto *brandSub = new QLabel(QStringLiteral("充电网络运营管理"), sidebar);
    brandSub->setObjectName(QStringLiteral("brandSub"));
    brandText->addWidget(brandTitle);
    brandText->addWidget(brandSub);
    brandRow->addLayout(brandText, 1);
    sidebarLayout->addLayout(brandRow);
    sidebarLayout->addSpacing(24);

    auto *sectionLabel = new QLabel(QStringLiteral("功能菜单"), sidebar);
    sectionLabel->setObjectName(QStringLiteral("sectionLabel"));
    sidebarLayout->addWidget(sectionLabel);
    sidebarLayout->addSpacing(6);

    m_nav = new QListWidget;
    m_nav->setObjectName(QStringLiteral("nav"));
    m_nav->setFrameShape(QFrame::NoFrame);
    m_nav->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_nav->addItems({QStringLiteral("营收分析"),
                     QStringLiteral("电桩状态总览"),
                     QStringLiteral("充电桩管理"),
                     QStringLiteral("充电站管理"),
                     QStringLiteral("用户管理")});
    sidebarLayout->addWidget(m_nav, 1);

    auto *accountCard = new QFrame(sidebar);
    accountCard->setObjectName(QStringLiteral("accountCard"));
    auto *accountLayout = new QHBoxLayout(accountCard);
    accountLayout->setContentsMargins(12, 10, 12, 10);
    accountLayout->setSpacing(8);
    auto *onlineDot = new QLabel(accountCard);
    onlineDot->setObjectName(QStringLiteral("onlineDot"));
    onlineDot->setFixedSize(8, 8);
    accountLayout->addWidget(onlineDot);
    auto *accountText = new QVBoxLayout;
    accountText->setSpacing(0);
    m_accountName = new QLabel(QStringLiteral("admin"), accountCard);
    m_accountName->setObjectName(QStringLiteral("accountName"));
    auto *accountRole = new QLabel(QStringLiteral("管理员 · 已登录"), accountCard);
    accountRole->setObjectName(QStringLiteral("accountRole"));
    accountText->addWidget(m_accountName);
    accountText->addWidget(accountRole);
    accountLayout->addLayout(accountText, 1);
    sidebarLayout->addWidget(accountCard);

    root->addWidget(sidebar);

    // 右侧内容区
    auto *pageHost = new QWidget(m_workspace);
    pageHost->setObjectName(QStringLiteral("pageHost"));
    auto *pageLayout = new QVBoxLayout(pageHost);
    pageLayout->setContentsMargins(20, 18, 20, 14);
    pageLayout->setSpacing(0);

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
    pageLayout->addWidget(m_pages, 1);
    root->addWidget(pageHost, 1);

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
        ncs::info(this, QStringLiteral("提示"),
                  QStringLiteral("请输入账号和密码"));
        return;
    }
    if (!PlatformService::adminLogin(account, password)) {
        ncs::warning(this, QStringLiteral("登录失败"),
                     QStringLiteral("账号或密码错误，请重新输入"));
        m_password->selectAll();
        m_password->setFocus();
        return;
    }

    m_adminAccount = account;
    m_accountName->setText(account);
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
