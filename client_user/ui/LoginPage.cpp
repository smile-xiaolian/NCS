#include "LoginPage.h"
#include <QVBoxLayout>
#include <QRandomGenerator>
#include "core/service/PlatformService.h"

LoginPage::LoginPage(QWidget *parent) : QWidget(parent) {
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(30, 60, 30, 40);
    layout->setSpacing(15);

    auto titleLabel = new QLabel("电动汽车充电服务");
    titleLabel->setStyleSheet("font-size: 22pt; font-weight: bold; color: #2b4c7e;");
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);

    auto subTitle = new QLabel("NCS 智能应用管理平台用户端");
    subTitle->setStyleSheet("color: #909399; font-size: 12pt; margin-bottom: 20px;");
    subTitle->setAlignment(Qt::AlignCenter);
    layout->addWidget(subTitle);

    phoneEdit = new QLineEdit;
    phoneEdit->setPlaceholderText("请输入 11 位手机号");
    codeEdit = new QLineEdit;
    codeEdit->setPlaceholderText("请输入 6 位验证码");

    getOtpBtn = new QPushButton("获取验证码");
    getOtpBtn->setObjectName("secondaryBtn");
    loginBtn = new QPushButton("登录 / 自动注册");

    hintLabel = new QLabel;
    hintLabel->setStyleSheet("color: #e6a23c; font-weight: bold;");
    hintLabel->setAlignment(Qt::AlignCenter);

    layout->addWidget(new QLabel("手机号："));
    layout->addWidget(phoneEdit);
    layout->addWidget(getOtpBtn);
    layout->addWidget(new QLabel("验证码："));
    layout->addWidget(codeEdit);
    layout->addWidget(hintLabel);
    layout->addSpacing(10);
    layout->addWidget(loginBtn);
    layout->addStretch();

    connect(getOtpBtn, &QPushButton::clicked, this, [this] {
        QString num = phoneEdit->text().trimmed();
        if (num.size() != 11 || !num.startsWith('1')) { return; }
        generatedOtp = QString::number(QRandomGenerator::global()->bounded(100000, 999999));
        hintLabel->setText("模拟验证码：" + generatedOtp + " (60秒有效)");
        otpCountdown = 60;
        getOtpBtn->setEnabled(false);
        getOtpBtn->setText(QString("%1秒重试").arg(otpCountdown));
        otpTimer.start(1000);
    });

    connect(&otpTimer, &QTimer::timeout, this, [this] {
        --otpCountdown;
        if (otpCountdown > 0) {
            getOtpBtn->setText(QString("%1秒重试").arg(otpCountdown));
            return;
        }
        otpTimer.stop();
        getOtpBtn->setEnabled(true);
        getOtpBtn->setText("获取验证码");
    });

    connect(loginBtn, &QPushButton::clicked, this, [this] {
        if (generatedOtp.isEmpty() || codeEdit->text() != generatedOtp) { return; }
        QString err;
        User u = PlatformService::loginOrRegister(phoneEdit->text(), &err);
        if (u.id > 0) {
            emit loginSuccess(u);
        }
    });
}

void LoginPage::clearInputs() {
    phoneEdit->clear();
    codeEdit->clear();
    hintLabel->clear();
    generatedOtp.clear();
}