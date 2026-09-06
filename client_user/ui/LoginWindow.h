#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QWidget>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include "core/models/User.h"

class LoginWindow : public QWidget
{
    Q_OBJECT
public:
    explicit LoginWindow(QWidget *parent = nullptr);

signals:
    void loginSuccess(const User &user);

private:
    QLineEdit *phoneEdit;
    QLineEdit *codeEdit;
    QLabel *hintLabel;
    QPushButton *getOtpBtn;
    QPushButton *loginBtn;

    QString currentOtp;
    QTimer otpTimer;
    int otpCountdown = 0;

    void onGetOtpClicked();
    void onLoginClicked();
};

#endif // LOGINWINDOW_H