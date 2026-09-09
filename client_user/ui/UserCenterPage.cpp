#include "UserCenterPage.h"
#include <QAbstractAnimation>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <QMenu>
#include <QLineEdit>
#include <QFrame>
#include "core/service/PlatformService.h"

UserCenterPage::UserCenterPage(QWidget *parent) : QWidget(parent)
{
    setStyleSheet("background-color: #f8fafc; font-family: 'Microsoft YaHei', sans-serif;");

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 20, 16, 20);
    mainLayout->setSpacing(16);

    // 1. 顶部个人名片 Card
    auto profileCard = new QFrame();
    profileCard->setStyleSheet(
        "QFrame {"
        "   background-color: #ffffff;"
        "   border: 1px solid #e2e8f0;"
        "   border-radius: 20px;"
        "}"
    );
    auto profileLayout = new QVBoxLayout(profileCard);
    profileLayout->setContentsMargins(20, 20, 20, 20);
    profileLayout->setSpacing(16);

    auto topHeaderLayout = new QHBoxLayout();
    avatarLabel = new QLabel();
    avatarLabel->setFixedSize(64, 64);

    auto nameCol = new QVBoxLayout();
    nicknameLabel = new QLabel();
    nicknameLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #0f172a; border: none; background: transparent;");

    phoneLabel = new QLabel();
    phoneLabel->setStyleSheet("font-size: 12px; color: #64748b; border: none; background: transparent;");

    nameCol->addWidget(nicknameLabel);
    nameCol->addWidget(phoneLabel);

    topHeaderLayout->addWidget(avatarLabel);
    topHeaderLayout->addSpacing(12);
    topHeaderLayout->addLayout(nameCol, 1);

    changeAvatarBtn = new QPushButton("更换头像");
    changeAvatarBtn->setStyleSheet(
        "QPushButton { background-color: #f1f5f9; color: #475569; font-size: 11px; font-weight: bold; border-radius: 12px; padding: 6px 12px; border: none; }"
        "QPushButton:hover { background-color: #e2e8f0; }"
    );
    topHeaderLayout->addWidget(changeAvatarBtn, 0, Qt::AlignTop);
    profileLayout->addLayout(topHeaderLayout);

    // 灰色分割线
    auto line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("color: #f1f5f9;");
    profileLayout->addWidget(line);

    // 账户资产仪表板（余额与状态）
    auto walletLayout = new QHBoxLayout();
    balanceLabel = new QLabel();
    balanceLabel->setStyleSheet("font-size: 13px; color: #334155; border: none; background: transparent;");

    debtLabel = new QLabel();
    debtLabel->setStyleSheet("font-size: 13px; border: none; background: transparent;");

    walletLayout->addWidget(balanceLabel);
    walletLayout->addStretch();
    walletLayout->addWidget(debtLabel);
    profileLayout->addLayout(walletLayout);

    mainLayout->addWidget(profileCard);

    // 2. 仿 App 设置菜单项组
    auto menuCard = new QFrame();
    menuCard->setStyleSheet(
        "QFrame {"
        "   background-color: #ffffff;"
        "   border: 1px solid #e2e8f0;"
        "   border-radius: 20px;"
        "}"
    );
    auto menuLayout = new QVBoxLayout(menuCard);
    menuLayout->setContentsMargins(0, 4, 0, 4);
    menuLayout->setSpacing(0);

    auto createMenuItem = [](const QString &iconAndTitle, const QString &textColor = "#0f172a") -> QPushButton* {
        auto btn = new QPushButton(iconAndTitle + "   ›");
        btn->setStyleSheet(QString(
            "QPushButton {"
            "   text-align: left;"
            "   padding: 16px 20px;"
            "   font-size: 14px;"
            "   font-weight: bold;"
            "   color: %1;"
            "   border: none;"
            "   background: transparent;"
            "}"
            "QPushButton:hover { background-color: #f8fafc; }"
            "QPushButton:pressed { background-color: #f1f5f9; }"
        ).arg(textColor));
        return btn;
    };

    editNickItemBtn = createMenuItem("✏️   修改用户昵称");
    rechargeItemBtn = createMenuItem("💳   账户充值与欠费补缴");
    logoutItemBtn   = createMenuItem("🚪   退出登录账号", "#ef4444");

    menuLayout->addWidget(editNickItemBtn);
    
    auto line1 = new QFrame(); line1->setFrameShape(QFrame::HLine); line1->setStyleSheet("color: #f8fafc;");
    menuLayout->addWidget(line1);

    menuLayout->addWidget(rechargeItemBtn);

    auto line2 = new QFrame(); line2->setFrameShape(QFrame::HLine); line2->setStyleSheet("color: #f8fafc;");
    menuLayout->addWidget(line2);

    menuLayout->addWidget(logoutItemBtn);

    mainLayout->addWidget(menuCard);
    mainLayout->addStretch();

    // 绑定信号槽
    connect(changeAvatarBtn, &QPushButton::clicked, this, &UserCenterPage::onChangeAvatar);
    connect(editNickItemBtn, &QPushButton::clicked, this, &UserCenterPage::openEditNicknameDialog);
    connect(rechargeItemBtn, &QPushButton::clicked, this, &UserCenterPage::openRechargeDialog);
    connect(logoutItemBtn, &QPushButton::clicked, this, &UserCenterPage::logoutRequested);
}

void UserCenterPage::setUser(const User &user)
{
    u = user;
    refreshProfile();
}

void UserCenterPage::refreshProfile()
{
    nicknameLabel->setText(u.nickname);
    phoneLabel->setText(QString("账号：%1****%2").arg(u.phone.left(3)).arg(u.phone.right(4)));
    balanceLabel->setText(QString("余额：<font color='#f59e0b'><b>¥ %1</b></font>").arg(u.balance, 0, 'f', 2));

    if (u.debt > 0) {
        debtLabel->setText(QString("欠费：<font color='#ef4444'><b>¥ %1</b></font>").arg(u.debt, 0, 'f', 2));
    } else {
        debtLabel->setText("状态：<font color='#10b981'><b>正常</b></font>");
    }

    QString avatarPath = QString("data/avatars/%1.png").arg(u.id);
    if (QFile::exists(avatarPath)) {
        avatarLabel->setPixmap(QPixmap(avatarPath).scaled(64, 64, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    } else {
        avatarLabel->setText("默认头像");
        avatarLabel->setStyleSheet("background-color: #cbd5e1; color: #475569; border-radius: 32px; font-size: 11px; qproperty-alignment: AlignCenter;");
    }
}

// 弹出现代圆角“修改昵称”对话框
void UserCenterPage::openEditNicknameDialog()
{
    auto dlg = new QDialog(this);
    dlg->setWindowTitle("修改昵称");
    dlg->setFixedSize(300, 190);
    dlg->setStyleSheet("QDialog { background-color: #ffffff; border-radius: 16px; }");

    auto layout = new QVBoxLayout(dlg);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(14);

    auto title = new QLabel("设置新的用户昵称", dlg);
    title->setStyleSheet("font-size: 15px; font-weight: bold; color: #0f172a;");
    
    auto input = new QLineEdit(dlg);
    input->setText(u.nickname);
    input->setStyleSheet(
        "QLineEdit { padding: 9px; border: 1.5px solid #e2e8f0; border-radius: 10px; font-size: 13px; background: #f8fafc; }"
        "QLineEdit:focus { border-color: #10b981; background: #ffffff; }"
    );

    auto saveBtn = new QPushButton("确认保存", dlg);
    saveBtn->setCursor(Qt::PointingHandCursor);
    saveBtn->setStyleSheet(
        "QPushButton { background-color: #10b981; color: white; font-weight: bold; padding: 10px; border-radius: 10px; border: none; }"
        "QPushButton:hover { background-color: #059669; }"
    );

    layout->addWidget(title);
    layout->addWidget(input);
    layout->addWidget(saveBtn);

    connect(saveBtn, &QPushButton::clicked, dlg, [&, dlg, input]() {
        QString newNick = input->text().trimmed();
        QString errorMsg;
        if (PlatformService::updateNickname(u.id, newNick, &errorMsg)) {
            u = PlatformService::loginOrRegister(u.phone);
            refreshProfile();
            emit userUpdated(u);
            QMessageBox::information(dlg, "提示", "昵称修改成功！");
            dlg->accept();
        } else {
            QMessageBox::warning(dlg, "提示", errorMsg);
        }
    });

    // --- 增加弹窗平滑放大进场动画 ---[cite: 9]
    dlg->show();
    auto anim = new QPropertyAnimation(dlg, "geometry");
    anim->setDuration(250);
    QRect startRect = dlg->geometry();
    // 从中心缩小状态放大[cite: 9]
    anim->setStartValue(QRect(startRect.x() + 20, startRect.y() + 20, startRect.width() - 40, startRect.height() - 40));
    anim->setEndValue(startRect);
    anim->setEasingCurve(QEasingCurve::OutBack); // 带有些许灵动回弹效果[cite: 9]
    anim->start(QAbstractAnimation::DeleteWhenStopped);

    dlg->exec();
}

// 弹出手机端风格“账户充值”对话框
void UserCenterPage::openRechargeDialog()
{
    QDialog dlg(this);
    dlg.setWindowTitle("账户充值");
    dlg.setFixedSize(320, 200);
    dlg.setStyleSheet("background-color: #ffffff;");

    auto layout = new QVBoxLayout(&dlg);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(12);

    auto title = new QLabel("请输入充值金额 (¥ 0.01 - 10000)：", &dlg);
    title->setStyleSheet("font-size: 13px; font-weight: bold; color: #1e293b;");
    
    auto input = new QLineEdit(&dlg);
    input->setPlaceholderText("例如: 100");
    input->setStyleSheet("padding: 8px; border: 1px solid #cbd5e1; border-radius: 6px; font-size: 13px;");

    auto payBtn = new QPushButton("立即支付", &dlg);
    payBtn->setStyleSheet("background-color: #10b981; color: white; font-weight: bold; padding: 9px; border-radius: 6px; border: none;");

    layout->addWidget(title);
    layout->addWidget(input);
    layout->addWidget(payBtn);

    connect(payBtn, &QPushButton::clicked, &dlg, [&]() {
        double amount = input->text().toDouble();
        QString errorMsg;
        if (PlatformService::recharge(u.id, amount, &errorMsg)) {
            u = PlatformService::loginOrRegister(u.phone);
            refreshProfile();
            emit userUpdated(u);
            QMessageBox::information(&dlg, "提示", "充值成功！");
            dlg.accept();
        } else {
            QMessageBox::warning(&dlg, "提示", errorMsg);
        }
    });

    dlg.exec();
}

void UserCenterPage::onChangeAvatar()
{
    QMenu menu(this);
    QAction *actionSelect = menu.addAction("从本地相册选择");
    QAction *actionCamera = menu.addAction("拍照上传头像");

    QAction *selectedAction = menu.exec(QCursor::pos());
    if (selectedAction == actionSelect) {
        selectLocalImage();
    } else if (selectedAction == actionCamera) {
        openCameraCapture();
    }
}

void UserCenterPage::selectLocalImage()
{
    QString fileName = QFileDialog::getOpenFileName(this, "选择头像", "", "图片文件 (*.png *.jpg *.jpeg)");
    if (fileName.isEmpty()) return;
    
    QImage img(fileName);
    if (img.isNull()) {
        QMessageBox::warning(this, "提示", "未能正常加载选择的图片");
        return;
    }
    
    saveAvatarImage(img);
}

void UserCenterPage::openCameraCapture()
{
    auto dialog = new QDialog(this);
    dialog->setWindowTitle("拍照上传头像");
    dialog->setFixedSize(360, 480);
    dialog->setAttribute(Qt::WA_DeleteOnClose, false);

    auto dialogLayout = new QVBoxLayout(dialog);

    auto videoWidget = new QVideoWidget(dialog);
    videoWidget->setStyleSheet("background-color: #000000; border-radius: 8px;");
    dialogLayout->addWidget(videoWidget, 1);

    auto previewLabel = new QLabel(dialog);
    previewLabel->setAlignment(Qt::AlignCenter);
    previewLabel->setStyleSheet("background-color: #000000; border-radius: 8px;");
    previewLabel->hide();
    dialogLayout->addWidget(previewLabel, 1);

    auto camera = new QCamera(dialog);
    auto captureSession = new QMediaCaptureSession(dialog);
    auto imageCapture = new QImageCapture(dialog);

    captureSession->setCamera(camera);
    captureSession->setVideoOutput(videoWidget);
    captureSession->setImageCapture(imageCapture);

    auto btnLayout = new QHBoxLayout();
    auto captureBtn = new QPushButton("拍照", dialog);
    captureBtn->setStyleSheet("background-color: #3b82f6; color: white; font-weight: bold; padding: 8px; border-radius: 6px;");

    auto retakeBtn = new QPushButton("重拍", dialog);
    retakeBtn->setObjectName("secondaryBtn");
    retakeBtn->hide();

    auto confirmBtn = new QPushButton("确认使用", dialog);
    confirmBtn->setStyleSheet("background-color: #10b981; color: white; font-weight: bold; padding: 8px; border-radius: 6px;");
    confirmBtn->hide();

    btnLayout->addWidget(captureBtn);
    btnLayout->addWidget(retakeBtn);
    btnLayout->addWidget(confirmBtn);
    dialogLayout->addLayout(btnLayout);

    QImage capturedImage;

    connect(imageCapture, &QImageCapture::imageCaptured, dialog, [&](int id, const QImage &preview) {
        Q_UNUSED(id);
        capturedImage = preview;

        camera->stop();
        captureSession->setVideoOutput(nullptr);
        
        videoWidget->hide();
        previewLabel->setPixmap(QPixmap::fromImage(preview).scaled(previewLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        previewLabel->show();

        captureBtn->hide();
        retakeBtn->show();
        confirmBtn->show();
    });

    connect(retakeBtn, &QPushButton::clicked, dialog, [&]() {
        previewLabel->hide();
        videoWidget->show();

        retakeBtn->hide();
        confirmBtn->hide();
        captureBtn->show();

        captureSession->setVideoOutput(videoWidget);
        camera->start();
    });

    connect(captureBtn, &QPushButton::clicked, dialog, [&]() {
        if (imageCapture->isReadyForCapture()) {
            imageCapture->capture();
        }
    });

    connect(confirmBtn, &QPushButton::clicked, dialog, [&]() {
        camera->stop();
        captureSession->setVideoOutput(nullptr);
        captureSession->setCamera(nullptr);
        captureSession->setImageCapture(nullptr);

        dialog->accept();
    });

    camera->start();

    if (dialog->exec() == QDialog::Accepted && !capturedImage.isNull()) {
        saveAvatarImage(capturedImage);
    }

    camera->stop();
    delete dialog;
}

void UserCenterPage::saveAvatarImage(const QImage &image)
{
    QDir().mkpath("data/avatars");
    QString destPath = QString("data/avatars/%1.png").arg(u.id);

    QImage scaledImg = image.scaled(200, 200, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    if (scaledImg.save(destPath, "PNG")) {
        refreshProfile();
        QMessageBox::information(this, "提示", "头像更换成功！");
    } else {
        QMessageBox::warning(this, "提示", "头像保存失败，请重试！");
    }
}
