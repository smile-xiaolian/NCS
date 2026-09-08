#ifndef USERCENTERPAGE_H
#define USERCENTERPAGE_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QCamera>
#include <QMediaCaptureSession>
#include <QImageCapture>
#include <QVideoWidget>
#include <QDialog>
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
    QLabel *nicknameLabel;
    QLabel *phoneLabel;
    QLabel *balanceLabel;
    QLabel *debtLabel;

    QPushButton *changeAvatarBtn;
    QPushButton *editNickItemBtn;
    QPushButton *rechargeItemBtn;
    QPushButton *logoutItemBtn;

    void refreshProfile();
    void onChangeAvatar();
    void selectLocalImage();
    void openCameraCapture();
    void saveAvatarImage(const QImage &image);

    // 弹窗逻辑
    void openEditNicknameDialog();
    void openRechargeDialog();
};

#endif // USERCENTERPAGE_H
