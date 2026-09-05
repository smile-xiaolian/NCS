#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include "core/models/User.h"

class LoginPage : public QWidget {
    Q_OBJECT
public:
    LoginPage(QWidget *parent = nullptr);
    void clearInputs();

signals:
    void loginSuccess(const User &user);

private:
    QLineEdit *phoneEdit;
    QLineEdit *codeEdit;
    QPushButton *getOtpBtn;
    QPushButton *loginBtn;
    QLabel *hintLabel;
    
    QTimer otpTimer;
    int otpCountdown = 0;
    QString generatedOtp;
};