#pragma once
#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include "core/models/User.h"

class ProfilePage : public QWidget {
    Q_OBJECT
public:
    ProfilePage(QWidget *parent = nullptr);
    void updateUserInfo(const User &user);

signals:
    void userInfoUpdated(const User &user);
    void logoutRequested();

private:
    QLabel *avatarLabel;
    QLabel *infoLabel;
    QLineEdit *nickEdit;
    QLineEdit *moneyEdit;
    QPushButton *saveNickBtn;
    QPushButton *payBtn;
    QPushButton *changeAvatarBtn;
    QPushButton *logoutBtn;
    
    User currentUser;
};