#ifndef USERCENTERPAGE_H
#define USERCENTERPAGE_H

#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include "core/models/User.h"

class UserCenterPage : public QWidget
{
    Q_OBJECT
public:
    explicit UserCenterPage(QWidget *parent = nullptr);
    void setUser(const User &user);

signals:
    void logoutRequested();
    void userUpdated(const User &updatedUser);

private:
    User u;

    QLabel *avatarLabel;
    QLabel *infoLabel;
    QLineEdit *nickEdit;
    QLineEdit *moneyEdit;

    QPushButton *changeAvatarBtn;
    QPushButton *saveNickBtn;
    QPushButton *payBtn;
    QPushButton *logoutBtn;

    void refreshProfile();
    void onChangeAvatar();
    void onSaveNick();
    void onPay();
};

#endif // USERCENTERPAGE_H