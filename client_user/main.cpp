
#include <QApplication>
#include <QWidget>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QTimer>
#include <QRandomGenerator>
#include <QDateTime>
#include <QDesktopServices>
#include <QUrl>
#include <QRegularExpression>
#include <QComboBox>
#include <QFile>
#include <QScrollArea>
#include <QFileDialog>
#include <QDir>
#include <QGraphicsDropShadowEffect>

#include "core/service/PlatformService.h"

class UserApp : public QWidget
{
    QStackedWidget *p = new QStackedWidget;

    // 底部导航按钮
    QPushButton *navHome = new QPushButton("首页");
    QPushButton *navOrders = new QPushButton("我的订单");
    QPushButton *navProfile = new QPushButton("个人中心");
    QWidget *bottomNavBar = new QWidget;

    // 登录控件
    QLineEdit *phone = new QLineEdit;
    QLineEdit *code = new QLineEdit;
    QLabel *hint = new QLabel;

    // 个人中心控件
    QLabel *avatarLabel = new QLabel;
    QLabel *info = new QLabel;
    QLineEdit *nick = new QLineEdit;
    QLineEdit *money = new QLineEdit;

    // 首页定位控件
    QComboBox *regionCombo = new QComboBox;
    QLineEdit *addressEdit = new QLineEdit;
    QPushButton *locateBtn = new QPushButton("定位");
    double currentLat = 31.2304;
    double currentLng = 121.4737;

    // 电站详情页专属标题
    QLabel *stationDetailTitle = new QLabel;
    // 充电控制/结算页专属状态
    QLabel *chargingStatusLabel = new QLabel;

    QTableWidget *st = new QTableWidget;
    QTableWidget *ch = new QTableWidget;
    QTableWidget *oh = new QTableWidget;

    User u;
    QString otp;
    QTimer otpTimer;
    int otpCountdown = 0;
    QTimer t;

    void setupTable(QTableWidget *w, QStringList h)
    {
        w->setColumnCount(h.size());
        w->setHorizontalHeaderLabels(h);
        w->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        w->setSelectionBehavior(QAbstractItemView::SelectRows);
        w->setEditTriggers(QAbstractItemView::NoEditTriggers);
        w->verticalHeader()->setVisible(false);
        w->setStyleSheet(
            "QTableWidget { background-color: #ffffff; alternate-background-color: #f9f9f9; border: 1px solid #e0e0e0; border-radius: 8px; gridline-color: #f0f0f0; }"
            "QHeaderView::section { background-color: #f5f7fa; padding: 8px; border: none; font-weight: bold; color: #333333; }"
        );
    }

    void msg(QString s)
    {
        QMessageBox::information(this, "NCS 充电桩", s);
    }

    // 检查是否存在未结算订单 (UC-U-06)
    bool checkUnsettledOrder(int &outOrderId)
    {
        auto ordersList = PlatformService::orders(u.id);
        for (auto &o : ordersList)
        {
            auto m = o.toMap();
            int stVal = m["status"].toInt();
            if (stVal == 0 || stVal == 1) // 0: 预约中, 1: 充电中
            {
                outOrderId = m["id"].toInt();
                return true;
            }
        }
        return false;
    }

    void updateNavStyle(int activeIndex)
    {
        QString activeStyle = "background-color: #2b4c7e; color: white; border-radius: 6px; font-weight: bold; padding: 8px;";
        QString normalStyle = "background-color: #f4f4f5; color: #606266; border-radius: 6px; padding: 8px;";

        navHome->setStyleSheet(activeIndex == 1 ? activeStyle : normalStyle);
        navOrders->setStyleSheet(activeIndex == 4 ? activeStyle : normalStyle);
        navProfile->setStyleSheet(activeIndex == 3 ? activeStyle : normalStyle);
    }

    void stations()
    {
        auto a = PlatformService::stations(currentLat, currentLng);
        st->setRowCount(a.size());

        for (int i = 0; i < a.size(); i++)
        {
            auto m = a[i].toMap();
            QStringList v = {
                m["name"].toString(),
                QString("%1 元/度").arg(m["price"].toDouble(), 0, 'f', 2),
                QString("%1 / %2 桩").arg(m["idle"].toInt()).arg(m["total"].toInt()),
                QString("%1 km").arg(m["distance"].toDouble(), 0, 'f', 1)
            };

            for (int j = 0; j < v.size(); j++)
            {
                auto item = new QTableWidgetItem(v[j]);
                if (j == 2) {
                    item->setForeground(m["idle"].toInt() > 0 ? QColor("#52c41a") : QColor("#ff4d4f"));
                }
                st->setItem(i, j, item);
            }
            st->item(i, 0)->setData(Qt::UserRole, m["id"]);
        }
    }

    void profile()
    {
        info->setText(
            QString("<b>昵称：</b>%1<br>"
                    "<b>手机号：</b>%2****%3<br>"
                    "<b>钱包余额：</b><span style='color: #fa8c16; font-size: 14pt;'>¥ %4</span><br>"
                    "<b>注册时间：</b>%5")
                .arg(u.nickname)
                .arg(u.phone.left(3))
                .arg(u.phone.right(4))
                .arg(u.balance, 0, 'f', 2)
                .arg(u.createdAt)
        );
        nick->setText(u.nickname);

        QString avatarPath = QString("data/avatars/%1.png").arg(u.id);
        if (QFile::exists(avatarPath)) {
            avatarLabel->setPixmap(QPixmap(avatarPath).scaled(70, 70, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
        } else {
            avatarLabel->setText("默认头像");
            avatarLabel->setStyleSheet("background-color: #e4e7ed; color: #909399; border-radius: 35px; font-size: 11px; qproperty-alignment: AlignCenter;");
        }
    }

    void orders()
    {
        auto a = PlatformService::orders(u.id);
        oh->setRowCount(a.size());
        QStringList ss = { "预约中", "充电中", "已完成", "已取消" };

        for (int i = 0; i < a.size(); i++)
        {
            auto m = a[i].toMap();
            int stVal = m["status"].toInt();
            QStringList v = {
                m["station_name"].toString(),
                m["charger_code"].toString(),
                ss.value(stVal),
                QString::number(m["energy"].toDouble(), 'f', 2) + " kWh",
                "¥ " + QString::number(m["amount"].toDouble(), 'f', 2),
                m["created_at"].toString()
            };

            for (int j = 0; j < v.size(); j++)
            {
                auto item = new QTableWidgetItem(v[j]);
                // 最新一条若未结算（预约中或充电中），文字高亮为绿色
                if (i == 0 && (stVal == 0 || stVal == 1)) {
                    item->setForeground(QColor("#52c41a"));
                    QFont f = item->font();
                    f.setBold(true);
                    item->setFont(f);
                }
                oh->setItem(i, j, item);
            }
            oh->item(i, 0)->setData(Qt::UserRole, m["id"]);
            oh->item(i, 0)->setData(Qt::UserRole + 1, stVal);
        }
    }

public:
    UserApp()
    {
        setWindowTitle("NCS 电动汽车充电应用管理平台（用户端）");
        setFixedSize(420, 760);

        // 全局基础样式
        setStyleSheet(
            "QWidget { font-family: 'Microsoft YaHei', sans-serif; font-size: 13px; color: #2c3e50; }"
            "QLineEdit { padding: 8px 12px; border: 1px solid #dcdfe6; border-radius: 6px; background: #ffffff; }"
            "QLineEdit:focus { border-color: #409eff; }"
            "QPushButton { background-color: #409eff; color: white; border: none; border-radius: 6px; padding: 9px 16px; font-weight: bold; }"
            "QPushButton:hover { background-color: #66b1ff; }"
            "QPushButton:pressed { background-color: #3a8ee6; }"
            "QPushButton#secondaryBtn { background-color: #f4f4f5; color: #606266; border: 1px solid #dcdfe6; }"
            "QPushButton#secondaryBtn:hover { background-color: #e4e7ed; }"
        );

        auto mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);

        mainLayout->addWidget(p);

        // 底部全局横向导航栏
        bottomNavBar->setFixedHeight(65);
        bottomNavBar->setStyleSheet("background-color: #ffffff; border-top: 1px solid #e4e7ed;");
        auto navLayout = new QHBoxLayout(bottomNavBar);
        navLayout->setContentsMargins(15, 8, 15, 8);
        navLayout->setSpacing(10);

        navLayout->addWidget(navHome);
        navLayout->addWidget(navOrders);
        navLayout->addWidget(navProfile);
        mainLayout->addWidget(bottomNavBar);
        bottomNavBar->hide(); // 默认登录页隐藏导航栏

        // ==================== 1. 登录页面 ====================
        auto loginPage = new QWidget;
        auto loginLayout = new QVBoxLayout(loginPage);
        loginLayout->setContentsMargins(30, 60, 30, 40);
        loginLayout->setSpacing(15);

        auto titleLabel = new QLabel("电动汽车充电服务");
        titleLabel->setStyleSheet("font-size: 22pt; font-weight: bold; color: #2b4c7e;");
        titleLabel->setAlignment(Qt::AlignCenter);
        loginLayout->addWidget(titleLabel);

        auto subTitle = new QLabel("NCS 智能应用管理平台用户端");
        subTitle->setStyleSheet("color: #909399; font-size: 12pt; margin-bottom: 20px;");
        subTitle->setAlignment(Qt::AlignCenter);
        loginLayout->addWidget(subTitle);

        phone->setPlaceholderText("请输入 11 位手机号");
        code->setPlaceholderText("请输入 6 位验证码");

        auto *getOtpBtn = new QPushButton("获取验证码");
        getOtpBtn->setObjectName("secondaryBtn");
        auto *loginBtn = new QPushButton("登录 / 自动注册");

        hint->setStyleSheet("color: #e6a23c; font-weight: bold;");
        hint->setAlignment(Qt::AlignCenter);

        loginLayout->addWidget(new QLabel("手机号："));
        loginLayout->addWidget(phone);
        loginLayout->addWidget(getOtpBtn);
        loginLayout->addWidget(new QLabel("验证码："));
        loginLayout->addWidget(code);
        loginLayout->addWidget(hint);
        loginLayout->addSpacing(10);
        loginLayout->addWidget(loginBtn);
        loginLayout->addStretch();

        p->addWidget(loginPage);

        // ==================== 2. 首页 (电站卡片列表) ====================
        auto homePage = new QWidget;
        auto homeLayout = new QVBoxLayout(homePage);
        homeLayout->setContentsMargins(15, 15, 15, 15);
        homeLayout->setSpacing(10);

        auto locateLayout = new QHBoxLayout();
        regionCombo->addItem("上海·人民广场", QVariant::fromValue(QPointF(31.2304, 121.4737)));
        regionCombo->addItem("上海·陆家嘴", QVariant::fromValue(QPointF(31.2393, 121.5000)));
        regionCombo->addItem("北京·天安门", QVariant::fromValue(QPointF(39.9042, 116.4074)));
        regionCombo->addItem("深圳·福田", QVariant::fromValue(QPointF(22.5431, 114.0579)));
        addressEdit->setPlaceholderText("搜索区域或详细地址");

        locateLayout->addWidget(regionCombo, 2);
        locateLayout->addWidget(addressEdit, 2);
        locateLayout->addWidget(locateBtn, 1);
        homeLayout->addLayout(locateLayout);

        auto listTitle = new QLabel("<b>附近优质充电站（点击卡片直接预约）</b>");
        homeLayout->addWidget(listTitle);

        setupTable(st, { "站点名称", "单价", "空闲/总数", "距离" });
        homeLayout->addWidget(st);

        auto *enterDetailBtn = new QPushButton("进入选桩详情页");
        homeLayout->addWidget(enterDetailBtn);

        p->addWidget(homePage);

        // ==================== 3. 电桩详情与预约页 (独立配色区分) ====================
        auto detailPage = new QWidget;
        detailPage->setStyleSheet("background-color: #f3f0ff;"); // 紫色
        auto detailLayout = new QVBoxLayout(detailPage);
        detailLayout->setContentsMargins(15, 15, 15, 15);

        stationDetailTitle->setStyleSheet("font-size: 15pt; font-weight: bold; color: #5b21b6; margin-bottom: 5px;");
        detailLayout->addWidget(stationDetailTitle);

        auto detailTip = new QLabel("请选择空闲电桩进行预约（余额需 ≥ 5 元）：");
        detailTip->setStyleSheet("color: #6b21a8; font-weight: bold;");
        detailLayout->addWidget(detailTip);

        setupTable(ch, { "编号", "类型", "功率", "状态" });
        detailLayout->addWidget(ch);

        auto *reserveBtn = new QPushButton("预约选中电桩");
        reserveBtn->setStyleSheet("background-color: #7c3aed; color: white; font-weight: bold; border-radius: 6px; padding: 10px;");
        auto *navigateBtn = new QPushButton("一键导航 (腾讯地图)");
        navigateBtn->setObjectName("secondaryBtn");
        auto *backHomeBtn = new QPushButton("返回首页");
        backHomeBtn->setObjectName("secondaryBtn");

        detailLayout->addWidget(reserveBtn);
        detailLayout->addWidget(navigateBtn);
        detailLayout->addWidget(backHomeBtn);

        p->addWidget(detailPage);

        // ==================== 4. 个人中心页面 (精简优化) ====================
        auto mePage = new QWidget;
        auto meOuterLayout = new QVBoxLayout(mePage);
        meOuterLayout->setContentsMargins(0, 0, 0, 0);

        auto scrollArea = new QScrollArea;
        scrollArea->setWidgetResizable(true);
        scrollArea->setFrameShape(QFrame::NoFrame);

        auto meScrollContent = new QWidget;
        auto meLayout = new QVBoxLayout(meScrollContent);
        meLayout->setContentsMargins(20, 20, 20, 20);
        meLayout->setSpacing(15);

        auto profileCardLayout = new QHBoxLayout();
        avatarLabel->setFixedSize(70, 70);
        avatarLabel->setAlignment(Qt::AlignCenter);

        auto *changeAvatarBtn = new QPushButton("更换头像");
        changeAvatarBtn->setObjectName("secondaryBtn");
        changeAvatarBtn->setMaximumWidth(90);

        auto leftAvatarCol = new QVBoxLayout();
        leftAvatarCol->addWidget(avatarLabel);
        leftAvatarCol->addWidget(changeAvatarBtn);
        leftAvatarCol->setAlignment(avatarLabel, Qt::AlignCenter);
        leftAvatarCol->setAlignment(changeAvatarBtn, Qt::AlignCenter);

        info->setStyleSheet("font-size: 13px; line-height: 1.6; color: #303133;");
        profileCardLayout->addLayout(leftAvatarCol);
        profileCardLayout->addSpacing(15);
        profileCardLayout->addWidget(info, 1);

        meLayout->addLayout(profileCardLayout);
        meLayout->addWidget(new QLabel("<hr style='border: none; border-top: 1px solid #ebeef5;'>"));

        meLayout->addWidget(new QLabel("<b>修改昵称：</b>"));
        auto nickLayout = new QHBoxLayout();
        nickLayout->addWidget(nick, 3);
        auto *saveNickBtn = new QPushButton("保存昵称");
        nickLayout->addWidget(saveNickBtn, 1);
        meLayout->addLayout(nickLayout);

        meLayout->addWidget(new QLabel("<b>账户充值：</b>"));
        money->setPlaceholderText("请输入充值金额 (0.01 - 10000)");
        meLayout->addWidget(money);
        auto *payBtn = new QPushButton("立即充值");
        meLayout->addWidget(payBtn);
        
        // 退出登录按钮
        auto *logoutBtn = new QPushButton("退出登录");
        logoutBtn->setObjectName("secondaryBtn"); // 使用次要按钮样式（灰色或边框风格）
        logoutBtn->setStyleSheet("background-color: #f56c6c; color: white; font-weight: bold; margin-top: 15px;"); // 颜色可再改
        meLayout->addWidget(logoutBtn);

        meLayout->addStretch();
        scrollArea->setWidget(meScrollContent);
        meOuterLayout->addWidget(scrollArea);

        p->addWidget(mePage);

        // ==================== 5. 我的订单页面 ====================
        auto histPage = new QWidget;
        auto histLayout = new QVBoxLayout(histPage);
        histLayout->setContentsMargins(15, 15, 15, 15);

        histLayout->addWidget(new QLabel("<b>历史订单与正在进行中订单（点击订单查看详情/去结算）</b>"));
        setupTable(oh, { "电站", "电桩", "状态", "电量", "金额", "时间" });
        histLayout->addWidget(oh);

        auto *viewOrderDetailsBtn = new QPushButton("查看/结算选中订单");
        histLayout->addWidget(viewOrderDetailsBtn);

        p->addWidget(histPage);

        // ==================== 6. 订单结算/充电控制页面 ====================
        auto settlePage = new QWidget;
        settlePage->setStyleSheet("background-color: #fdf6ec;"); // 独立柔和暖色背景
        auto settleLayout = new QVBoxLayout(settlePage);
        settleLayout->setContentsMargins(20, 20, 20, 20);
        settleLayout->setSpacing(15);

        auto settleTitle = new QLabel("<b>充电控制与订单结算</b>");
        settleTitle->setStyleSheet("font-size: 16pt; color: #e6a23c; font-weight: bold;");
        settleLayout->addWidget(settleTitle);

        chargingStatusLabel->setStyleSheet("background-color: #ffffff; padding: 15px; border-radius: 8px; border: 1px solid #f3d19e; color: #e6a23c; font-weight: bold; font-size: 14px;");
        chargingStatusLabel->setWordWrap(true);
        settleLayout->addWidget(chargingStatusLabel);

        auto *startChargingBtn = new QPushButton("开始充电");
        startChargingBtn->setStyleSheet("background-color: #67c23a; color: white; font-weight: bold; padding: 12px; border-radius: 6px;");
        
        auto *settleOrderBtn = new QPushButton("结束充电并结算订单");
        settleOrderBtn->setStyleSheet("background-color: #f56c6c; color: white; font-weight: bold; padding: 12px; border-radius: 6px;");

        auto *backHomeFromSettle = new QPushButton("返回首页");
        backHomeFromSettle->setObjectName("secondaryBtn");

        settleLayout->addWidget(startChargingBtn);
        settleLayout->addWidget(settleOrderBtn);
        settleLayout->addWidget(backHomeFromSettle);
        settleLayout->addStretch();

        p->addWidget(settlePage); // 索引 5


        // ==================== 核心逻辑与信号槽绑定 ====================

        // 底部导航栏联动切换
        connect(navHome, &QPushButton::clicked, this, [this] {
            stations();
            p->setCurrentIndex(1);
            updateNavStyle(1);
        });
        connect(navOrders, &QPushButton::clicked, this, [this] {
            orders();
            p->setCurrentIndex(4);
            updateNavStyle(4);
        });
        connect(navProfile, &QPushButton::clicked, this, [this] {
            profile();
            p->setCurrentIndex(3);
            updateNavStyle(3);
        });

        // 更换头像 (UC-U-05)
        connect(changeAvatarBtn, &QPushButton::clicked, this, [this]
        {
            QString fileName = QFileDialog::getOpenFileName(this, "选择头像", "", "图片文件 (*.png *.jpg *.jpeg)");
            if (fileName.isEmpty()) return;
            QFile file(fileName);
            if (file.size() > 5 * 1024 * 1024) { msg("图片过大，请选择 5MB 以内图片"); return; }

            QDir().mkpath("data/avatars");
            QString destPath = QString("data/avatars/%1.png").arg(u.id);
            if (QFile::exists(destPath)) QFile::remove(destPath);
            if (file.copy(destPath)) { profile(); msg("头像更换成功"); }
            else { msg("头像保存失败"); }
        });

        // 定位按钮
        connect(locateBtn, &QPushButton::clicked, this, [this]
        {
            QPointF coords = regionCombo->currentData().toPointF();
            currentLat = coords.x();
            currentLng = coords.y();
            if (!addressEdit->text().trimmed().isEmpty()) {
                msg(QString("提示：已使用“%1”的预置坐标进行测算。").arg(regionCombo->currentText()));
            }
            stations();
        });

        // 验证码
        connect(getOtpBtn, &QPushButton::clicked, this, [this, getOtpBtn]
        {
            const QString num = phone->text().trimmed();
            if (num.size() != 11 || !num.startsWith('1')) { msg("请输入正确的 11 位手机号"); return; }
            otp = QString::number(QRandomGenerator::global()->bounded(100000, 999999));
            hint->setText("模拟验证码：" + otp + " (60秒有效)");
            otpCountdown = 60;
            getOtpBtn->setEnabled(false);
            getOtpBtn->setText(QString("%1秒重试").arg(otpCountdown));
            otpTimer.start(1000);
        });

        connect(&otpTimer, &QTimer::timeout, this, [this, getOtpBtn]
        {
            --otpCountdown;
            if (otpCountdown > 0) { getOtpBtn->setText(QString("%1秒重试").arg(otpCountdown)); return; }
            otpTimer.stop();
            getOtpBtn->setEnabled(true);
            getOtpBtn->setText("获取验证码");
        });

        // 登录/自动注册 -> 成功后显示底部导航并进入首页
        connect(loginBtn, &QPushButton::clicked, this, [this]
        {
            if (otp.isEmpty()) { msg("请先获取验证码"); return; }
            if (code->text() != otp) { msg("验证码错误"); return; }

            QString e;
            u = PlatformService::loginOrRegister(phone->text(), &e);
            if (!u.id) { msg(e); return; }

            stations();
            p->setCurrentIndex(1);
            bottomNavBar->show(); // 登录成功显示底部导航
            updateNavStyle(1);
        });

        // 点击首页电站卡片 / 按钮触发：未结算订单拦截 + 进入电桩详情页 (UC-U-06)
        auto handleStationClick = [this]
        {
            auto x = st->currentItem();
            if (!x) { msg("请先选择一个充电站"); return; }

            // 拦截检查：是否存在未结算订单
            int dummyId = 0;
            if (checkUnsettledOrder(dummyId))
            {
                QMessageBox::warning(this, "提示", "您有未完成的充电订单，请先结算");
                p->setCurrentIndex(5); // 强制跳转到结算/控制页
                return;
            }

            // 无未结算订单，正常进入电桩详情页
            int stationId = st->item(x->row(), 0)->data(Qt::UserRole).toInt();
            auto sInfo = PlatformService::station(stationId);
            stationDetailTitle->setText(sInfo["name"].toString() + " - 电桩列表");

            auto a = PlatformService::chargers(stationId);
            ch->setRowCount(a.size());
            for (int i = 0; i < a.size(); i++)
            {
                auto z = a[i].toMap();
                int stStatus = z["status"].toInt();
                QString statusStr = stStatus == 0 ? "空闲" : (stStatus == 1 ? "使用中" : "故障");
                QStringList v = { z["code"].toString(), z["type"].toString(), QString::number(z["power"].toDouble()) + " kW", statusStr };

                for (int j = 0; j < 4; j++)
                {
                    auto item = new QTableWidgetItem(v[j]);
                    if (j == 3) {
                        item->setForeground(stStatus == 0 ? QColor("#52c41a") : (stStatus == 1 ? QColor("#fa8c16") : QColor("#ff4d4f")));
                    }
                    ch->setItem(i, j, item);
                }
                ch->item(i, 0)->setData(Qt::UserRole, z["id"]);
            }
            p->setCurrentIndex(2);
        };

        connect(st, &QTableWidget::doubleClicked, this, handleStationClick);
        connect(enterDetailBtn, &QPushButton::clicked, this, handleStationClick);

        // 预约电桩 (UC-U-07)
        connect(reserveBtn, &QPushButton::clicked, this, [this]
        {
            auto x = ch->currentItem();
            if (!x) { msg("请选择电桩"); return; }

            QString e;
            if (PlatformService::reserve(u.id, ch->item(x->row(), 0)->data(Qt::UserRole).toInt(), &e))
            {
                msg("预约成功！即将进入充电控制与结算页");
                p->setCurrentIndex(5); // 进入充电控制页
            }
            else { msg(e); }
        });

        // 导航
        connect(navigateBtn, &QPushButton::clicked, this, [this]
        {
            auto x = st->currentItem();
            if (!x) return;
            auto s = PlatformService::station(st->item(x->row(), 0)->data(Qt::UserRole).toInt());
            QDesktopServices::openUrl(QUrl(QString("https://apis.map.qq.com/uri/v1/routeplan?type=drive&to=%1&coord=%2,%3").arg(s["name"].toString()).arg(s["latitude"].toDouble()).arg(s["longitude"].toDouble())));
        });

        connect(backHomeBtn, &QPushButton::clicked, this, [this] { stations(); p->setCurrentIndex(1); updateNavStyle(1); });

        // 保存昵称
        connect(saveNickBtn, &QPushButton::clicked, this, [this]
        {
            QString e;
            if (PlatformService::updateNickname(u.id, nick->text(), &e))
            {
                u = PlatformService::loginOrRegister(u.phone);
                profile();
                msg("昵称修改成功");
            }
            else { msg(e); }
        });

        // 充值
        connect(payBtn, &QPushButton::clicked, this, [this]
        {
            QString e;
            if (PlatformService::recharge(u.id, money->text().toDouble(), &e))
            {
                u = PlatformService::loginOrRegister(u.phone);
                profile();
                msg("充值成功");
            }
            else { msg(e); }
        });
        
        // 点击退出登录按钮的逻辑
        connect(logoutBtn, &QPushButton::clicked, this, [this]
        {
            u = User(); // 清空当前登录用户信息
            phone->clear(); // 清空登录页手机号输入框
            code->clear();  // 清空验证码输入框
            hint->clear();  // 清空提示
            
            bottomNavBar->hide(); // 隐藏底部全局导航栏
            p->setCurrentIndex(0); // 切换回登录页面（索引 0）
            msg("已安全退出登录");
        });

        // 在“我的订单”页面点击查看订单详情/去结算
        connect(viewOrderDetailsBtn, &QPushButton::clicked, this, [this]
        {
            auto x = oh->currentItem();
            if (!x) { msg("请选择一条订单"); return; }
            int stVal = oh->item(x->row(), 0)->data(Qt::UserRole + 1).toInt();
            
            if (stVal == 0 || stVal == 1) {
                p->setCurrentIndex(5); // 未完成则去结算/控制页
            } else {
                msg("该订单已完结，可在上方查看历史明细小票。");
            }
        });

        // 结算页：点击“开始充电”
        connect(startChargingBtn, &QPushButton::clicked, this, [this]
        {
            QString e;
            if (PlatformService::start(u.id, &e)) {
                msg("已成功开始充电！");
            } else {
                msg(e.isEmpty() ? "启动失败，请确认是否有有效预约订单" : e);
            }
        });

        // 结算页：点击“结束充电并结算订单” (UC-U-09)
        connect(settleOrderBtn, &QPushButton::clicked, this, [this]
        {
            QString e;
            if (PlatformService::settle(u.id, &e)) {
                msg("结算完成！费用已从余额扣除。");
                u = PlatformService::loginOrRegister(u.phone);
                stations();
                p->setCurrentIndex(1);
                updateNavStyle(1);
            } else {
                msg(e.isEmpty() ? "当前没有进行中的订单可结算" : e);
            }
        });

        connect(backHomeFromSettle, &QPushButton::clicked, this, [this] { stations(); p->setCurrentIndex(1); updateNavStyle(1); });

        // 定时刷新实时计费与状态 (UC-U-08)
        t.start(1000);
        connect(&t, &QTimer::timeout, this, [this]
        {
            if (p->currentIndex() != 5) return; // 仅在结算/控制页实时计算

            bool hasActive = false;
            for (auto &o : PlatformService::orders(u.id))
            {
                auto z = o.toMap();
                int stVal = z["status"].toInt();
                if (stVal == 1) // 充电中
                {
                    hasActive = true;
                    int sec = QDateTime::fromString(z["start_time"].toString(), "yyyy-MM-dd HH:mm:ss").secsTo(QDateTime::currentDateTime()) * 60;
                    double en = z["power"].toDouble() * sec / 3600.;
                    double cost = en * z["price"].toDouble();
                    chargingStatusLabel->setText(
                        QString("[充电中 - 60x演示加速]<br>"
                                "• 充电时长：%1 秒<br>"
                                "• 累计电量：%2 度 (kWh)<br>"
                                "• 实时费用：¥ %3")
                            .arg(sec).arg(en, 0, 'f', 2).arg(cost, 0, 'f', 2)
                    );
                    break;
                }
                else if (stVal == 0) // 预约中
                {
                    hasActive = true;
                    chargingStatusLabel->setText("状态：已预约电桩，请点击上方“开始充电”以启动计时。");
                    break;
                }
            }
            if (!hasActive) {
                chargingStatusLabel->setText("✅ 当前暂无活跃的预约或充电订单。");
            }
        });
    }
};

int main(int c, char **v)
{
    QApplication a(c, v);
    QString e;

    if (!PlatformService::initialize(&e))
    {
        QMessageBox::critical(nullptr, "NCS 错误", e);
        return 1;
    }

    UserApp w;
    w.show();

    return a.exec();
}