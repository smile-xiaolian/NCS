#include "LoginWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QRandomGenerator>
#include <QMessageBox>
#include <QEasingCurve>
#include "core/service/PlatformService.h"

LoginWindow::LoginWindow(QWidget *parent) : QWidget(parent)
{
    setupUI();

    connect(getOtpBtn, &QPushButton::clicked, this, &LoginWindow::onGetOtpClicked);
    connect(loginBtn, &QPushButton::clicked, this, &LoginWindow::onLoginClicked);

    // 验证码倒计时逻辑[cite: 9]
    connect(&otpTimer, &QTimer::timeout, this, [this] {
        --otpCountdown;
        if (otpCountdown > 0) {
            getOtpBtn->setText(QString("%1s 重试").arg(otpCountdown));
        } else {
            otpTimer.stop();
            getOtpBtn->setEnabled(true);
            getOtpBtn->setText("获取验证码");
        }
    });
}

void LoginWindow::setupUI()
{
    setStyleSheet("QWidget { font-family: 'Microsoft YaHei', sans-serif; background-color: #f8fafc; }");

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(28, 50, 28, 40);
    mainLayout->setSpacing(24);

    // =========================================================================
    // 1. 顶部 Header 品牌与名称显示区域 (个性化排版)[cite: 9]
    // =========================================================================
    headerContainer = new QWidget(this);
    auto headerLayout = new QVBoxLayout(headerContainer);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(8);

    // (1) 顶部科技感小徽章 (Badge)
    auto badgeLabel = new QLabel("⚡ SMART CHARGING EV");
    badgeLabel->setStyleSheet(
        "QLabel {"
        "   color: #10b981;"
        "   background-color: #ecfdf5;"
        "   font-size: 11px;"
        "   font-weight: bold;"
        "   border-radius: 10px;"
        "   padding: 4px 10px;"
        "}"
    );
    badgeLabel->setFixedWidth(150);

    // (2) 个性化大标题 (采用绿蓝科技色彩体系)
    auto titleLabel = new QLabel("极速充电", headerContainer);
    titleLabel->setStyleSheet(
        "font-size: 32px;"
        "font-weight: 800;"
        "color: #0f172a;"
        "letter-spacing: 1px;"
    );

    // (3) 带有优雅字距修饰的副标题
    auto subTitle = new QLabel("NCS 智能电动汽车充电平台", headerContainer);
    subTitle->setStyleSheet(
        "font-size: 13px;"
        "font-weight: 500;"
        "color: #64748b;"
        "letter-spacing: 1px;"
    );

    headerLayout->addWidget(badgeLabel);
    headerLayout->addSpacing(4);
    headerLayout->addWidget(titleLabel);
    headerLayout->addWidget(subTitle);

    mainLayout->addWidget(headerContainer);

    // =========================================================================
    // 2. 表单输入 Card 容器 (手机 App 胶囊交互风格)[cite: 9]
    // =========================================================================
    formContainer = new QWidget(this);
    auto formLayout = new QVBoxLayout(formContainer);
    formLayout->setContentsMargins(0, 0, 0, 0);
    formLayout->setSpacing(16);

    // 手机号胶囊输入框
    phoneEdit = new QLineEdit;
    phoneEdit->setPlaceholderText("请输入 11 位手机号码");
    phoneEdit->setStyleSheet(
        "QLineEdit {"
        "   background-color: #ffffff;"
        "   border: 1.5px solid #e2e8f0;"
        "   border-radius: 14px;"
        "   padding: 12px 16px;"
        "   font-size: 14px;"
        "   color: #0f172a;"
        "}"
        "QLineEdit:focus {"
        "   border-color: #10b981;"
        "   background-color: #ffffff;"
        "}"
    );

    // 验证码输入组合行（包含右侧悬浮“获取验证码”按钮）
    auto codeContainer = new QWidget();
    auto codeLayout = new QHBoxLayout(codeContainer);
    codeLayout->setContentsMargins(0, 0, 0, 0);
    codeLayout->setSpacing(10);

    codeEdit = new QLineEdit;
    codeEdit->setPlaceholderText("请输入 6 位验证码");
    codeEdit->setStyleSheet(
        "QLineEdit {"
        "   background-color: #ffffff;"
        "   border: 1.5px solid #e2e8f0;"
        "   border-radius: 14px;"
        "   padding: 12px 16px;"
        "   font-size: 14px;"
        "   color: #0f172a;"
        "}"
        "QLineEdit:focus {"
        "   border-color: #10b981;"
        "}"
    );

    getOtpBtn = new QPushButton("获取验证码");
    getOtpBtn->setCursor(Qt::PointingHandCursor);
    getOtpBtn->setStyleSheet(
        "QPushButton {"
        "   background-color: #f1f5f9;"
        "   color: #0f172a;"
        "   font-weight: bold;"
        "   font-size: 12px;"
        "   border-radius: 14px;"
        "   padding: 12px 16px;"
        "   border: none;"
        "}"
        "QPushButton:hover { background-color: #e2e8f0; }"
        "QPushButton:disabled { background-color: #f8fafc; color: #cbd5e1; }"
    );

    codeLayout->addWidget(codeEdit, 1);
    codeLayout->addWidget(getOtpBtn);

    // 提示信息标签
    hintLabel = new QLabel;
    hintLabel->setStyleSheet("color: #f59e0b; font-size: 12px; font-weight: bold;");
    hintLabel->setAlignment(Qt::AlignLeft);

    // 登录 / 注册主大按钮
    loginBtn = new QPushButton("一键登录 / 自动注册");
    loginBtn->setCursor(Qt::PointingHandCursor);
    loginBtn->setStyleSheet(
        "QPushButton {"
        "   background-color: #10b981;"
        "   color: #ffffff;"
        "   font-size: 15px;"
        "   font-weight: bold;"
        "   border-radius: 16px;"
        "   padding: 14px;"
        "   border: none;"
        "}"
        "QPushButton:hover { background-color: #059669; }"
        "QPushButton:pressed { background-color: #047857; }"
    );

    formLayout->addWidget(phoneEdit);
    formLayout->addWidget(codeContainer);
    formLayout->addWidget(hintLabel);
    formLayout->addSpacing(10);
    formLayout->addWidget(loginBtn);

    mainLayout->addWidget(formContainer);
    mainLayout->addStretch();
}

// =========================================================================
// 3. 动态淡入 + 上下滑行动画设计 (Animation)[cite: 9]
// =========================================================================
void LoginWindow::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    startEntranceAnimation();
}

void LoginWindow::startEntranceAnimation()
{
    // 如果已有动画组在运行则先停止释放
    if (animGroup) {
        animGroup->stop();
        delete animGroup;
        animGroup = nullptr;
    }

    animGroup = new QParallelAnimationGroup(this);

    // --- 顶部 Header 透明度 + 滑行动画 ---[cite: 9]
    headerOpacity = new QGraphicsOpacityEffect(headerContainer);
    headerContainer->setGraphicsEffect(headerOpacity);

    auto headerFade = new QPropertyAnimation(headerOpacity, "opacity");
    headerFade->setDuration(600);
    headerFade->setStartValue(0.0);
    headerFade->setEndValue(1.0);
    headerFade->setEasingCurve(QEasingCurve::OutCubic);

    auto headerMove = new QPropertyAnimation(headerContainer, "pos");
    headerMove->setDuration(600);
    QPoint headerPos = headerContainer->pos();
    headerMove->setStartValue(QPoint(headerPos.x(), headerPos.y() - 30));
    headerMove->setEndValue(headerPos);
    headerMove->setEasingCurve(QEasingCurve::OutCubic);

    // --- 中部 Form 表单透明度 + 向上滑动动画 ---[cite: 9]
    formOpacity = new QGraphicsOpacityEffect(formContainer);
    formContainer->setGraphicsEffect(formOpacity);

    auto formFade = new QPropertyAnimation(formOpacity, "opacity");
    formFade->setDuration(700);
    formFade->setStartValue(0.0);
    formFade->setEndValue(1.0);
    formFade->setEasingCurve(QEasingCurve::OutCubic);

    auto formMove = new QPropertyAnimation(formContainer, "pos");
    formMove->setDuration(700);
    QPoint formPos = formContainer->pos();
    formMove->setStartValue(QPoint(formPos.x(), formPos.y() + 40));
    formMove->setEndValue(formPos);
    formMove->setEasingCurve(QEasingCurve::OutCubic);

    // 将动画并列组合在一起执行[cite: 9]
    animGroup->addAnimation(headerFade);
    animGroup->addAnimation(headerMove);
    animGroup->addAnimation(formFade);
    animGroup->addAnimation(formMove);

    animGroup->start();
}

void LoginWindow::onGetOtpClicked()
{
    const QString num = phoneEdit->text().trimmed();
    if (num.size() != 11 || !num.startsWith('1')) {
        QMessageBox::information(this, "提示", "请输入正确的 11 位手机号");
        return;
    }
    currentOtp = QString::number(QRandomGenerator::global()->bounded(100000, 999999));
    hintLabel->setText("模拟验证码：" + currentOtp + " (60秒有效)");
    otpCountdown = 60;
    getOtpBtn->setEnabled(false);
    getOtpBtn->setText(QString("%1s 重试").arg(otpCountdown));
    otpTimer.start(1000);
}

void LoginWindow::onLoginClicked()
{
    if (currentOtp.isEmpty()) {
        QMessageBox::information(this, "提示", "请先获取验证码");
        return;
    }
    if (codeEdit->text() != currentOtp) {
        QMessageBox::information(this, "提示", "验证码错误");
        return;
    }

    QString errorMsg;
    User u = PlatformService::loginOrRegister(phoneEdit->text(), &errorMsg);
    if (!u.id) {
        QMessageBox::information(this, "提示", errorMsg);
        return;
    }

    emit loginSuccess(u);
}
