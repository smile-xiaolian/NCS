#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QWidget>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QGraphicsOpacityEffect>
#include "core/models/User.h"

class LoginWindow : public QWidget
{
    Q_OBJECT
public:
    explicit LoginWindow(QWidget *parent = nullptr);

signals:
    void loginSuccess(const User &user);

protected:
    // 重写 showEvent，在界面每次显示时播放优雅的淡入滑行动画
    void showEvent(QShowEvent *event) override;

private:
    // UI 核心元素
    QWidget *headerContainer;
    QWidget *formContainer;

    QLineEdit *phoneEdit;
    QLineEdit *codeEdit;
    QLabel *hintLabel;
    QPushButton *getOtpBtn;
    QPushButton *loginBtn;

    // 动画控制对象
    QParallelAnimationGroup *animGroup = nullptr;
    QGraphicsOpacityEffect *headerOpacity = nullptr;
    QGraphicsOpacityEffect *formOpacity = nullptr;

    QString currentOtp;
    QTimer otpTimer;
    int otpCountdown = 0;

    void setupUI();
    void startEntranceAnimation(); // 进场动画实现
    void onGetOtpClicked();
    void onLoginClicked();
};

#endif // LOGINWINDOW_H
