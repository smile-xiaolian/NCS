#include "UserCenterPage.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include "core/service/PlatformService.h"

UserCenterPage::UserCenterPage(QWidget *parent) : QWidget(parent)
{
    auto outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);

    auto scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    auto contentWidget = new QWidget;
    auto layout = new QVBoxLayout(contentWidget);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(15);

    auto profileCardLayout = new QHBoxLayout();
    avatarLabel = new QLabel;
    avatarLabel->setFixedSize(70, 70);
    avatarLabel->setAlignment(Qt::AlignCenter);

    changeAvatarBtn = new QPushButton("更换头像");
    changeAvatarBtn->setObjectName("secondaryBtn");
    changeAvatarBtn->setMaximumWidth(90);

    auto avatarCol = new QVBoxLayout();
    avatarCol->addWidget(avatarLabel);
    avatarCol->addWidget(changeAvatarBtn);
    avatarCol->setAlignment(avatarLabel, Qt::AlignCenter);
    avatarCol->setAlignment(changeAvatarBtn, Qt::AlignCenter);

    infoLabel = new QLabel;
    infoLabel->setStyleSheet("font-size: 13px; line-height: 1.6; color: #303133;");

    profileCardLayout->addLayout(avatarCol);
    profileCardLayout->addSpacing(15);
    profileCardLayout->addWidget(infoLabel, 1);

    layout->addLayout(profileCardLayout);
    layout->addWidget(new QLabel("<hr style='border: none; border-top: 1px solid #ebeef5;'>"));

    layout->addWidget(new QLabel("<b>修改昵称：</b>"));
    auto nickLayout = new QHBoxLayout();
    nickEdit = new QLineEdit;
    saveNickBtn = new QPushButton("保存昵称");
    nickLayout->addWidget(nickEdit, 3);
    nickLayout->addWidget(saveNickBtn, 1);
    layout->addLayout(nickLayout);

    layout->addWidget(new QLabel("<b>账户充值（含欠费补缴）：</b>"));
    moneyEdit = new QLineEdit;
    moneyEdit->setPlaceholderText("请输入充值金额 (0.01 - 10000)");
    payBtn = new QPushButton("立即充值");
    layout->addWidget(moneyEdit);
    layout->addWidget(payBtn);

    logoutBtn = new QPushButton("退出登录");
    logoutBtn->setObjectName("secondaryBtn");
    logoutBtn->setStyleSheet("background-color: #f56c6c; color: white; font-weight: bold; margin-top: 15px;");
    layout->addWidget(logoutBtn);

    layout->addStretch();
    scrollArea->setWidget(contentWidget);
    outerLayout->addWidget(scrollArea);

    connect(changeAvatarBtn, &QPushButton::clicked, this, &UserCenterPage::onChangeAvatar);
    connect(saveNickBtn, &QPushButton::clicked, this, &UserCenterPage::onSaveNick);
    connect(payBtn, &QPushButton::clicked, this, &UserCenterPage::onPay);
    connect(logoutBtn, &QPushButton::clicked, this, &UserCenterPage::logoutRequested);
}

void UserCenterPage::setUser(const User &user)
{
    u = user;
    refreshProfile();
}

void UserCenterPage::refreshProfile()
{
    QString debtHtml = (u.debt > 0)
        ? QString("<br><b>欠费金额：</b><span style='color: #ff4d4f; font-size: 14pt;'>¥ %1 (需补缴)</span>").arg(u.debt, 0, 'f', 2)
        : "<br><b>欠费状态：</b><span style='color: #52c41a;'>无欠费</span>";

    infoLabel->setText(
        QString("<b>昵称：</b>%1<br>"
                "<b>手机号：</b>%2****%3<br>"
                "<b>钱包余额：</b><span style='color: #fa8c16; font-size: 14pt;'>¥ %4</span>"
                "%5<br>"
                "<b>注册时间：</b>%6")
            .arg(u.nickname)
            .arg(u.phone.left(3))
            .arg(u.phone.right(4))
            .arg(u.balance, 0, 'f', 2)
            .arg(debtHtml)
            .arg(u.createdAt)
    );
    nickEdit->setText(u.nickname);

    QString avatarPath = QString("data/avatars/%1.png").arg(u.id);
    if (QFile::exists(avatarPath)) {
        avatarLabel->setPixmap(QPixmap(avatarPath).scaled(70, 70, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    } else {
        avatarLabel->setText("默认头像");
        avatarLabel->setStyleSheet("background-color: #e4e7ed; color: #909399; border-radius: 35px; font-size: 11px; qproperty-alignment: AlignCenter;");
    }
}

void UserCenterPage::onChangeAvatar()
{
    QString fileName = QFileDialog::getOpenFileName(this, "选择头像", "", "图片文件 (*.png *.jpg *.jpeg)");
    if (fileName.isEmpty()) return;
    QFile file(fileName);
    if (file.size() > 5 * 1024 * 1024) {
        QMessageBox::information(this, "提示", "图片过大，请选择 5MB 以内图片");
        return;
    }

    QDir().mkpath("data/avatars");
    QString destPath = QString("data/avatars/%1.png").arg(u.id);
    if (QFile::exists(destPath)) QFile::remove(destPath);
    if (file.copy(destPath)) {
        refreshProfile();
        QMessageBox::information(this, "提示", "头像更换成功");
    } else {
        QMessageBox::information(this, "提示", "头像保存失败");
    }
}

void UserCenterPage::onSaveNick()
{
    QString errorMsg;
    if (PlatformService::updateNickname(u.id, nickEdit->text(), &errorMsg)) {
        u = PlatformService::loginOrRegister(u.phone);
        refreshProfile();
        emit userUpdated(u);
        QMessageBox::information(this, "提示", "昵称修改成功");
    } else {
        QMessageBox::warning(this, "提示", errorMsg);
    }
}

void UserCenterPage::onPay()
{
    QString errorMsg;
    if (PlatformService::recharge(u.id, moneyEdit->text().toDouble(), &errorMsg)) {
        u = PlatformService::loginOrRegister(u.phone);
        refreshProfile();
        moneyEdit->clear();
        emit userUpdated(u);
        QMessageBox::information(this, "提示", "充值/补缴成功");
    } else {
        QMessageBox::warning(this, "提示", errorMsg);
    }
}