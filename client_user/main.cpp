#include <QApplication>
#include <QWidget>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
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

#include "core/service/PlatformService.h"

class UserApp : public QWidget
{
    QStackedWidget *p = new QStackedWidget;

    QLineEdit *phone = new QLineEdit;
    QLineEdit *code = new QLineEdit;
    QLineEdit *nick = new QLineEdit;
    QLineEdit *money = new QLineEdit;

    // UC-U-02 定位区控件
    QComboBox *regionCombo = new QComboBox;
    QLineEdit *addressEdit = new QLineEdit;
    QPushButton *locateBtn = new QPushButton("定位");

    // 当前定位经纬度（默认：人民广场）
    double currentLat = 31.2304;
    double currentLng = 121.4737;

    QLabel *hint = new QLabel;
    QLabel *info = new QLabel;
    QLabel *charging = new QLabel;

    QTableWidget *st = new QTableWidget;
    QTableWidget *ch = new QTableWidget;
    QTableWidget *oh = new QTableWidget;

    User u;
    QString otp;

    // 验证码发送后的 60 秒倒计时
    QTimer otpTimer;
    int otpCountdown = 0;

    // 充电状态刷新定时器
    QTimer t;

    void setup(QTableWidget *w, QStringList h)
    {
        w->setColumnCount(h.size());
        w->setHorizontalHeaderLabels(h);
        w->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        w->setSelectionBehavior(QAbstractItemView::SelectRows);
        w->setEditTriggers(QAbstractItemView::NoEditTriggers);
    }

    void msg(QString s)
    {
        QMessageBox::information(this, "NCS", s);
    }

    void stations()
    {
        // 传递当前经纬度以利用 Haversine 公式计算实际距离并升序排列
        auto a = PlatformService::stations(currentLat, currentLng);

        st->setRowCount(a.size());

        for (int i = 0; i < a.size(); i++)
        {
            auto m = a[i].toMap();

            QStringList v = {
                m["name"].toString(),
                QString("%1 元/度").arg(m["price"].toDouble(), 0, 'f', 2),
                QString("%1/%2").arg(m["idle"].toInt()).arg(m["total"].toInt()),
                QString("%1 km").arg(m["distance"].toDouble(), 0, 'f', 1)
            };

            for (int j = 0; j < v.size(); j++)
            {
                st->setItem(i, j, new QTableWidgetItem(v[j]));
            }

            st->item(i, 0)->setData(Qt::UserRole, m["id"]);
        }
    }

    void profile()
    {
        info->setText(
            QString("%1\n手机号：%2****%3\n钱包余额：¥ %4\n注册时间：%5")
                .arg(u.nickname, u.phone.left(3), u.phone.right(4))
                .arg(u.balance, 0, 'f', 2)
                .arg(u.createdAt)
        );

        nick->setText(u.nickname);
    }

    void orders()
    {
        auto a = PlatformService::orders(u.id);

        oh->setRowCount(a.size());

        QStringList ss = {
            "预约",
            "充电中",
            "已完成",
            "已取消"
        };

        for (int i = 0; i < a.size(); i++)
        {
            auto m = a[i].toMap();

            QStringList v = {
                m["station_name"].toString(),
                m["charger_code"].toString(),
                ss.value(m["status"].toInt()),
                QString::number(m["energy"].toDouble(), 'f', 2),
                QString::number(m["amount"].toDouble(), 'f', 2),
                m["created_at"].toString()
            };

            for (int j = 0; j < v.size(); j++)
            {
                oh->setItem(i, j, new QTableWidgetItem(v[j]));
            }
        }
    }

public:
    UserApp()
    {
        setWindowTitle("NCS 充电用户端");
        setFixedSize(420, 760);

        auto r = new QVBoxLayout(this);
        r->addWidget(p);

        // 登录页面
        auto login = new QWidget;
        auto l = new QVBoxLayout(login);

        l->addWidget(new QLabel("<h1>电动汽车充电服务</h1>"));

        phone->setPlaceholderText("11 位手机号");
        code->setPlaceholderText("6 位验证码");

        auto *get = new QPushButton("获取验证码");
        auto *go = new QPushButton("登录 / 自动注册");

        l->addWidget(phone);
        l->addWidget(get);
        l->addWidget(code);
        l->addWidget(hint);
        l->addWidget(go);
        l->addStretch();

        p->addWidget(login);

        // 首页 (实现 UC-U-02 附近充电站查询与定位区)
        auto home = new QWidget;
        auto h = new QVBoxLayout(home);

        // 顶部定位区布局：区域下拉框 + 地址输入框 + 定位按钮
        auto locateLayout = new QHBoxLayout();
        regionCombo->addItem("上海·人民广场", QVariant::fromValue(QPointF(31.2304, 121.4737)));
        regionCombo->addItem("上海·陆家嘴", QVariant::fromValue(QPointF(31.2393, 121.5000)));
        regionCombo->addItem("北京·天安门", QVariant::fromValue(QPointF(39.9042, 116.4074)));
        regionCombo->addItem("深圳·福田中心", QVariant::fromValue(QPointF(22.5431, 114.0579)));

        addressEdit->setPlaceholderText("请输入详细地址");

        locateLayout->addWidget(regionCombo, 2);
        locateLayout->addWidget(addressEdit, 3);
        locateLayout->addWidget(locateBtn, 1);

        h->addLayout(locateLayout);
        h->addWidget(new QLabel("附近充电站列表："));

        setup(st, {
            "站点",
            "单价",
            "空闲/总数",
            "距离"
        });

        h->addWidget(st);

        auto *choose = new QPushButton("查看电桩 / 预约");
        auto *mine = new QPushButton("我的");
        auto *history = new QPushButton("我的订单");

        h->addWidget(choose);
        h->addWidget(mine);
        h->addWidget(history);

        p->addWidget(home);

        // 电桩详情 / 预约页面
        auto detail = new QWidget;
        auto d = new QVBoxLayout(detail);

        d->addWidget(new QLabel("选择空闲电桩后预约（余额至少 5 元）"));

        setup(ch, {
            "编号",
            "类型",
            "功率",
            "状态"
        });

        d->addWidget(ch);

        auto *reserve = new QPushButton("预约选中电桩");
        auto *navigate = new QPushButton("一键导航（浏览器）");
        auto *back = new QPushButton("返回站点");

        d->addWidget(reserve);
        d->addWidget(navigate);
        d->addWidget(back);

        p->addWidget(detail);

        // 我的页面
        auto me = new QWidget;
        auto m = new QVBoxLayout(me);

        m->addWidget(info);
        m->addWidget(nick);

        auto *save = new QPushButton("保存昵称");

        money->setPlaceholderText("充值金额 0.01 - 10000");

        auto *pay = new QPushButton("充值");
        auto *start = new QPushButton("开始 / 结束充电");
        auto *mb = new QPushButton("返回首页");

        m->addWidget(save);
        m->addWidget(money);
        m->addWidget(pay);
        m->addWidget(start);
        m->addWidget(charging);
        m->addWidget(mb);
        m->addStretch();

        p->addWidget(me);

        // 订单历史页面
        auto hist = new QWidget;
        auto o = new QVBoxLayout(hist);

        setup(oh, {
            "电站",
            "电桩",
            "状态",
            "电量",
            "金额",
            "创建时间"
        });

        o->addWidget(oh);

        auto *ob = new QPushButton("返回首页");
        o->addWidget(ob);

        p->addWidget(hist);

        // 点击“定位”按钮触发逻辑 (UC-U-02)
        connect(locateBtn, &QPushButton::clicked, this, [this]
        {
            // 目前由于尚未配置真实腾讯地图 Key 且满足需求退化逻辑：
            // 直接采用区域下拉框对应的预置经纬度作为当前位置。
            QPointF coords = regionCombo->currentData().toPointF();
            currentLat = coords.x();
            currentLng = coords.y();

            QString customAddr = addressEdit->text().trimmed();
            if (!customAddr.isEmpty())
            {
                // 若用户输入了地址，模拟提示由于未配置 Key 已退化使用预置坐标
                msg(QString("提示：未配置腾讯地图 Key 或网络不可用，已使用“%1”的预置坐标进行定位。")
                    .arg(regionCombo->currentText()));
            }

            // 重新刷新电站列表计算距离
            stations();
        });

        // 获取验证码
        connect(get, &QPushButton::clicked, this, [this, get]
        {
            const QString number = phone->text().trimmed();

            if (number.size() != 11 ||
                !number.startsWith('1') ||
                number.contains(QRegularExpression("[^0-9]")))
            {
                msg("请输入正确的 11 位手机号");
                return;
            }

            otp = QString::number(
                QRandomGenerator::global()->bounded(100000, 999999)
            );

            hint->setText("模拟验证码：" + otp + "（60 秒有效）");

            otpCountdown = 60;
            get->setEnabled(false);
            get->setText(QString("%1 秒后重新获取").arg(otpCountdown));

            otpTimer.start(1000);
        });

        // 验证码 60 秒倒计时
        connect(&otpTimer, &QTimer::timeout, this, [this, get]
        {
            --otpCountdown;

            if (otpCountdown > 0)
            {
                get->setText(QString("%1 秒后重新获取").arg(otpCountdown));
                return;
            }

            otpTimer.stop();
            get->setEnabled(true);
            get->setText("获取验证码");
        });

        // 登录 / 自动注册
        connect(go, &QPushButton::clicked, this, [this]
        {
            if (otp.isEmpty())
            {
                msg("请先获取验证码");
                return;
            }

            if (code->text() != otp)
            {
                msg("验证码错误");
                return;
            }

            QString e;
            u = PlatformService::loginOrRegister(phone->text(), &e);

            if (!u.id)
            {
                msg(e);
                return;
            }

            stations();
            p->setCurrentIndex(1);
        });

        // 查看电桩 / 预约
        connect(choose, &QPushButton::clicked, this, [this]
        {
            auto x = st->currentItem();

            if (!x)
            {
                msg("请选择电站");
                return;
            }

            auto a = PlatformService::chargers(
                st->item(x->row(), 0)->data(Qt::UserRole).toInt()
            );

            ch->setRowCount(a.size());

            for (int i = 0; i < a.size(); i++)
            {
                auto z = a[i].toMap();

                QStringList v = {
                    z["code"].toString(),
                    z["type"].toString(),
                    QString::number(z["power"].toDouble()),
                    z["status"].toInt() == 0
                        ? "空闲"
                        : z["status"].toInt() == 1
                            ? "使用中"
                            : "故障"
                };

                for (int j = 0; j < 4; j++)
                {
                    ch->setItem(i, j, new QTableWidgetItem(v[j]));
                }

                ch->item(i, 0)->setData(Qt::UserRole, z["id"]);
            }

            p->setCurrentIndex(2);
        });

        // 预约选中电桩
        connect(reserve, &QPushButton::clicked, this, [this]
        {
            auto x = ch->currentItem();

            if (!x)
            {
                msg("请选择电桩");
                return;
            }

            QString e;

            if (PlatformService::reserve(
                    u.id,
                    ch->item(x->row(), 0)->data(Qt::UserRole).toInt(),
                    &e))
            {
                msg("预约成功，请到“我的”开始充电");
            }
            else
            {
                msg(e);
            }
        });

        // 一键导航
        connect(navigate, &QPushButton::clicked, this, [this]
        {
            auto x = st->currentItem();

            if (!x)
            {
                return;
            }

            auto s = PlatformService::station(
                st->item(x->row(), 0)->data(Qt::UserRole).toInt()
            );

            QDesktopServices::openUrl(
                QUrl(
                    QString(
                        "https://apis.map.qq.com/uri/v1/routeplan?"
                        "type=drive&to=%1&coord=%2,%3"
                    )
                        .arg(s["name"].toString())
                        .arg(s["latitude"].toDouble())
                        .arg(s["longitude"].toDouble())
                )
            );
        });

        // 返回站点
        connect(back, &QPushButton::clicked, this, [this]
        {
            stations();
            p->setCurrentIndex(1);
        });

        // 我的
        connect(mine, &QPushButton::clicked, this, [this]
        {
            profile();
            p->setCurrentIndex(3);
        });

        // 我的订单
        connect(history, &QPushButton::clicked, this, [this]
        {
            orders();
            p->setCurrentIndex(4);
        });

        // 我的页面 -> 返回首页
        connect(mb, &QPushButton::clicked, this, [this]
        {
            stations();
            p->setCurrentIndex(1);
        });

        // 订单历史 -> 返回首页
        connect(ob, &QPushButton::clicked, this, [this]
        {
            p->setCurrentIndex(1);
        });

        // 保存昵称
        connect(save, &QPushButton::clicked, this, [this]
        {
            QString e;

            if (PlatformService::updateNickname(
                    u.id,
                    nick->text(),
                    &e))
            {
                u = PlatformService::loginOrRegister(u.phone);
                profile();
            }
            else
            {
                msg(e);
            }
        });

        // 模拟充值
        connect(pay, &QPushButton::clicked, this, [this]
        {
            QString e;

            if (PlatformService::recharge(
                    u.id,
                    money->text().toDouble(),
                    &e))
            {
                u = PlatformService::loginOrRegister(u.phone);
                profile();
                msg("支付成功");
            }
            else
            {
                msg(e);
            }
        });

        // 开始 / 结束充电
        connect(start, &QPushButton::clicked, this, [this]
        {
            QString e;
            bool act = false;
            bool res = false;

            for (auto &o : PlatformService::orders(u.id))
            {
                act |= o.toMap()["status"].toInt() == 1;
                res |= o.toMap()["status"].toInt() == 0;
            }

            bool ok = act
                ? PlatformService::settle(u.id, &e)
                : res
                    ? PlatformService::start(u.id, &e)
                    : false;

            if (ok)
            {
                msg(act ? "结算完成" : "已开始充电");
                u = PlatformService::loginOrRegister(u.phone);
                profile();
            }
            else
            {
                msg(e.isEmpty() ? "请先预约电桩" : e);
            }
        });

        // 定时刷新充电状态
        t.start(1000);

        connect(&t, &QTimer::timeout, this, [this]
        {
            for (auto &o : PlatformService::orders(u.id))
            {
                auto z = o.toMap();

                if (z["status"].toInt() == 1)
                {
                    int sec =
                        QDateTime::fromString(
                            z["start_time"].toString(),
                            "yyyy-MM-dd HH:mm:ss"
                        )
                            .secsTo(QDateTime::currentDateTime())
                        * 60;

                    double en =
                        z["power"].toDouble() * sec / 3600.;

                    charging->setText(
                        QString(
                            "充电中（60×演示加速）%1 秒 | %2 度 | ¥ %3"
                        )
                            .arg(sec)
                            .arg(en, 0, 'f', 2)
                            .arg(
                                en * z["price"].toDouble(),
                                0,
                                'f',
                                2
                            )
                    );

                    return;
                }
            }

            charging->setText("当前没有充电中的订单");
        });
    }
};

int main(int c, char **v)
{
    QApplication a(c, v);

    // 从外部文件加载样式表
    QFile styleFile("style.qss");
    if (styleFile.open(QFile::ReadOnly | QFile::Text))
    {
        a.setStyleSheet(styleFile.readAll());
    }

    QString e;

    if (!PlatformService::initialize(&e))
    {
        QMessageBox::critical(nullptr, "NCS错误", e);
        return 1;
    }

    UserApp w;
    w.show();

    return a.exec();
}
