#include "ProfilePage.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QFileDialog>
#include <QDir>
#include <QMessageBox>
#include "core/service/PlatformService.h"

ProfilePage::ProfilePage(QWidget *parent) : QWidget(parent) {
    auto meOuterLayout = new QVBoxLayout(this);
    meOuterLayout->setContentsMargins(0, 0, 0, 0);

    auto scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    auto meScrollContent = new QWidget;
    auto meLayout = new QVBoxLayout(meScrollContent);
    meLayout->setContentsMargins(20, 20, 20, 20);
    meLayout->setSpacing(15);

    auto profileCardLayout = new QHBoxLayout();
    avatarLabel = new QLabel;
    avatarLabel->setFixedSize(70, 70);
    avatarLabel->setAlignment(Qt::AlignCenter);

    changeAvatarBtn = new QPushButton("更换头像");
    changeAvatarBtn->setObjectName("secondaryBtn");
    changeAvatarBtn->setMaximumWidth(90);

    auto leftAvatarCol = new QVBoxLayout();
    leftAvatarCol->addWidget(avatarLabel);
    leftAvatarCol->addWidget(changeAvatarBtn);
    leftAvatarCol->setAlignment(avatarLabel, Qt::AlignCenter);
    leftAvatarCol->setAlignment(changeAvatarBtn, Qt::AlignCenter);

    infoLabel = new QLabel;
    infoLabel->setStyleSheet("font-size: 13px; line-height: 1.6; color: #303133;");
    profileCardLayout->addLayout(leftAvatarCol);
    profileCardLayout->addSpacing(15);
    profileCardLayout->addWidget(infoLabel, 1);

    meLayout->addLayout(profileCardLayout);
    meLayout->addWidget(new QLabel("<hr style='border: none; border-top: 1px solid #ebeef5;'>"));

    meLayout->addWidget(new QLabel("<b>修改昵称：</b>"));
    auto nickLayout = new QHBoxLayout();
    nickEdit = new QLineEdit;
    nickLayout->addWidget(nickEdit, 3);
    saveNickBtn = new QPushButton("保存昵称");
    nickLayout->addWidget(saveNickBtn, 1);
    meLayout->addLayout(nickLayout);

    meLayout->addWidget(new QLabel("<b>账户充值：</b>"));
    moneyEdit = new QLineEdit;
    moneyEdit->setPlaceholderText("请输入充值金额 (0.01 - 10000)");
    meLayout->addWidget(moneyEdit);
    payBtn = new QPushButton("立即充值");
    meLayout->addWidget(payBtn);

    // 退出登录按钮
    logoutBtn = new QPushButton("退出登录");
    logoutBtn->setStyleSheet("background-color: #f56c6c; color: white; font-weight: bold; margin-top: 15px; border-radius: 6px; padding: 10px;");
    meLayout->addWidget(logoutBtn);

    meLayout->addStretch();
    scrollArea->setWidget(meScrollContent);
    meOuterLayout->addWidget(scrollArea);

    connect(changeAvatarBtn, &QPushButton::clicked, this, [this] {
        QString fileName = QFileDialog::getOpenFileName(this, "选择头像", "", "图片文件 (*.png *.jpg *.jpeg)");
        if (fileName.isEmpty()) return;
        QFile file(fileName);
        if (file.size() > 5 * 1024 * 1024) { QMessageBox::information(this, "提示", "图片过大，请选择 5MB 以内图片"); return; }
        QDir().mkpath("avatars");
        QString destPath = QString("avatars/%1.png").arg(currentUser.id);
        if (QFile::exists(destPath)) QFile::remove(destPath);
        if (file.copy(destPath)) { updateUserInfo(currentUser); QMessageBox::information(this, "提示", "头像更换成功"); }
    });

    connect(saveNickBtn, &QPushButton::clicked, this, [this] {
        QString err;
        if (PlatformService::updateNickname(currentUser.id, nickEdit->text(), &err)) {
            currentUser = PlatformService::loginOrRegister(currentUser.phone);
            updateUserInfo(currentUser);
            emit userInfoUpdated(currentUser);
            QMessageBox::information(this, "提示", "昵称修改成功");
        } else { QMessageBox::information(this, "提示", err); }
    });

    connect(payBtn, &QPushButton::clicked, this, [this] {
        QString err;
        if (PlatformService::recharge(currentUser.id, moneyEdit->text().toDouble(), &err)) {
            currentUser = PlatformService::loginOrRegister(currentUser.phone);
            updateUserInfo(currentUser);
            emit userInfoUpdated(currentUser);
            QMessageBox::information(this, "提示", "充值成功");
        } else { QMessageBox::information(this, "提示", err); }
    });

    connect(logoutBtn, &QPushButton::clicked, this, &ProfilePage::logoutRequested);
}

void ProfilePage::updateUserInfo(const User &user) {
    currentUser = user;
    infoLabel->setText(
        QString("<b>昵称：</b>%1<br><b>手机号：</b>%2****%3<br><b>钱包余额：</b><span style='color: #fa8c16; font-size: 14pt;'>¥ %4</span><br><b>注册时间：</b>%5")
            .arg(currentUser.nickname).arg(currentUser.phone.left(3)).arg(currentUser.phone.right(4))
            .arg(currentUser.balance, 0, 'f', 2).arg(currentUser.createdAt)
    );
    nickEdit->setText(currentUser.nickname);

    QString avatarPath = QString("avatars/%1.png").arg(currentUser.id);
    if (QFile::exists(avatarPath)) {
        avatarLabel->setPixmap(QPixmap(avatarPath).scaled(70, 70, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    } else {
        avatarLabel->setText("默认头像");
        avatarLabel->setStyleSheet("background-color: #e4e7ed; color: #909399; border-radius: 35px; font-size: 11px; qproperty-alignment: AlignCenter;");
    }
}