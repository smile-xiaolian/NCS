#include <QApplication>
#include <QWidget>
#include <QStackedWidget>
#include <QVBoxLayout>
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

#include "core/service/PlatformService.h"

class UserApp : public QWidget
{
    QStackedWidget *p = new QStackedWidget;

    QLineEdit *phone = new QLineEdit;
    QLineEdit *code = new QLineEdit;
    QLineEdit *nick = new QLineEdit;
    QLineEdit *money = new QLineEdit;

    QLabel *hint = new QLabel;
    QLabel *info = new QLabel;
    QLabel *charging = new QLabel;

    QTableWidget *st = new QTableWidget;
    QTableWidget *ch = new QTableWidget;
    QTableWidget *oh = new QTableWidget;

    User u;
    QString otp;
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
        auto a = PlatformService::stations();

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

        auto *get = new QPushButton("获取验证码（模拟）");
        auto *go = new QPushButton("登录 / 自动注册");

        l->addWidget(phone);
        l->addWidget(get);
        l->addWidget(code);
        l->addWidget(hint);
        l->addWidget(go);
        l->addStretch();

        p->addWidget(login);

        // 首页
        auto home = new QWidget;
        auto h = new QVBoxLayout(home);

        h->addWidget(new QLabel("附近充电站（默认定位：人民广场）"));

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

        auto *pay = new QPushButton("模拟充值");
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

        // 获取验证码
        // UC-U-01 / BR-01：
        // 只有输入合法的 11 位手机号后，才允许获取验证码。
        // 手机号格式校验在点击“获取验证码”时触发，不在输入过程中打扰用户。
        connect(get, &QPushButton::clicked, this, [this]
        {
            const QString number = phone->text().trimmed();

            // 手机号必须为 11 位数字，且以 1 开头。
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

    QString e;

    if (!PlatformService::initialize(&e))
    {
        QMessageBox::critical(nullptr, "NCS", e);
        return 1;
    }

    UserApp w;
    w.show();

    return a.exec();
}