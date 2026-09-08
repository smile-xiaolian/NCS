#include "UserCenterPage.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <QMenu>
#include <QDialog>
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

// 弹出菜单选择上传方式
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

// 1. 本地相册选择逻辑
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

// 调起摄像头拍照流程（含内存安全防护、定格、重拍与确认交互）
void UserCenterPage::openCameraCapture()
{
    // 1. 创建对话框
    auto dialog = new QDialog(this);
    dialog->setWindowTitle("拍照上传头像");
    dialog->setFixedSize(360, 480);
    dialog->setAttribute(Qt::WA_DeleteOnClose, false); // 阻止提前自动销毁

    auto dialogLayout = new QVBoxLayout(dialog);

    // 2. 视频预览控件
    auto videoWidget = new QVideoWidget(dialog);
    videoWidget->setStyleSheet("background-color: #000000; border-radius: 8px;");
    dialogLayout->addWidget(videoWidget, 1);

    // 3. 定格截图显示 Label
    auto previewLabel = new QLabel(dialog);
    previewLabel->setAlignment(Qt::AlignCenter);
    previewLabel->setStyleSheet("background-color: #000000; border-radius: 8px;");
    previewLabel->hide();
    dialogLayout->addWidget(previewLabel, 1);

    // 4. 初始化多媒体底层对象
    auto camera = new QCamera(dialog);
    auto captureSession = new QMediaCaptureSession(dialog);
    auto imageCapture = new QImageCapture(dialog);

    captureSession->setCamera(camera);
    captureSession->setVideoOutput(videoWidget);
    captureSession->setImageCapture(imageCapture);

    // 5. 按钮布局
    auto btnLayout = new QHBoxLayout();
    auto captureBtn = new QPushButton("拍照", dialog);
    captureBtn->setStyleSheet("background-color: #409eff; color: white; font-weight: bold; padding: 8px; border-radius: 6px;");

    auto retakeBtn = new QPushButton("重拍", dialog);
    retakeBtn->setObjectName("secondaryBtn");
    retakeBtn->hide();

    auto confirmBtn = new QPushButton("确认使用", dialog);
    confirmBtn->setStyleSheet("background-color: #67c23a; color: white; font-weight: bold; padding: 8px; border-radius: 6px;");
    confirmBtn->hide();

    btnLayout->addWidget(captureBtn);
    btnLayout->addWidget(retakeBtn);
    btnLayout->addWidget(confirmBtn);
    dialogLayout->addLayout(btnLayout);

    QImage capturedImage;

    // 6. 拍照完成回调
    connect(imageCapture, &QImageCapture::imageCaptured, dialog, [&](int id, const QImage &preview) {
        Q_UNUSED(id);
        capturedImage = preview;

        // 停止摄像头预览，切换至图片显示
        camera->stop();
        captureSession->setVideoOutput(nullptr); // 解绑视频输出，防止后台线程继续踩内存
        
        videoWidget->hide();
        previewLabel->setPixmap(QPixmap::fromImage(preview).scaled(previewLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        previewLabel->show();

        captureBtn->hide();
        retakeBtn->show();
        confirmBtn->show();
    });

    // 7. 点击重拍
    connect(retakeBtn, &QPushButton::clicked, dialog, [&]() {
        previewLabel->hide();
        videoWidget->show();

        retakeBtn->hide();
        confirmBtn->hide();
        captureBtn->show();

        // 重新挂载视频输出并启动
        captureSession->setVideoOutput(videoWidget);
        camera->start();
    });

    // 8. 点击拍照
    connect(captureBtn, &QPushButton::clicked, dialog, [&]() {
        if (imageCapture->isReadyForCapture()) {
            imageCapture->capture();
        }
    });

    // 9. 点击确认使用（核心安全退出逻辑）
    connect(confirmBtn, &QPushButton::clicked, dialog, [&]() {
        // 安全关闭摄像头与会话
        camera->stop();
        captureSession->setVideoOutput(nullptr);
        captureSession->setCamera(nullptr);
        captureSession->setImageCapture(nullptr);

        dialog->accept();
    });

    // 启动摄像头预览
    camera->start();

    // 阻塞运行对话框
    if (dialog->exec() == QDialog::Accepted && !capturedImage.isNull()) {
        saveAvatarImage(capturedImage);
    }

    // 清理资源
    camera->stop();
    delete dialog; // 手动安全释放
}

// 统一保存头像图片
void UserCenterPage::saveAvatarImage(const QImage &image)
{
    QDir().mkpath("data/avatars");
    QString destPath = QString("data/avatars/%1.png").arg(u.id);

    // 格式化裁切并保存为正方形 PNG 格式
    QImage scaledImg = image.scaled(200, 200, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    if (scaledImg.save(destPath, "PNG")) {
        refreshProfile();
        QMessageBox::information(this, "提示", "头像更换成功！");
    } else {
        QMessageBox::warning(this, "提示", "头像保存失败，请重试！");
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
