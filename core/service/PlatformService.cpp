#include "PlatformService.h"
#include "core/database/DatabaseManager.h"
#include "core/utils/Haversine.h"
#include <QStandardPaths>
#include <QDir>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QCryptographicHash>
#include <QDateTime>
#include <QRegularExpression>
#include <algorithm>

namespace {

// 获取当前格式化时间字符串
QString now() {
    return QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
}

// 将 QSqlQuery 当前行转换为 QVariantMap 字典
QVariantMap map(QSqlQuery &q) {
    QVariantMap m;
    auto r = q.record();
    for (int i = 0; i < r.count(); ++i) {
        m[r.fieldName(i)] = q.value(i);
    }
    return m;
}

// 生成演示历史订单数据（用于图表和订单展示）
void seedDemoHistory() {
    QSqlQuery q(DatabaseManager::instance().db());
    q.exec("SELECT count(*) FROM charging_order WHERE status=2");
    q.next();
    if (q.value(0).toInt()) return;

    q.prepare("SELECT id FROM user WHERE phone=?");
    q.addBindValue("13800138000");
    q.exec();
    
    int uid = 0;
    if (q.next()) {
        uid = q.value(0).toInt();
    } else {
        q.prepare("INSERT INTO user(phone,nickname,balance,status,created_at) VALUES(?,?,?,?,?)");
        q.addBindValue("13800138000");
        q.addBindValue("演示车主");
        q.addBindValue(10000.0);
        q.addBindValue(1);
        q.addBindValue(now());
        q.exec();
        uid = q.lastInsertId().toInt();
    }

    q.exec("SELECT c.id, c.power, s.price FROM charger c JOIN station s ON s.id=c.station_id WHERE c.status<>2 ORDER BY c.id");
    QVariantList cs;
    while (q.next()) {
        cs << map(q);
    }
    if (cs.isEmpty()) return;

    DatabaseManager::instance().beginTransaction();
    for (int d = 30; d > 0; --d) {
        for (int n = 0; n < 12; ++n) {
            auto c = cs[(d + n) % cs.size()].toMap();
            auto end = QDateTime::currentDateTime().addDays(-d).addSecs(n * 3600);
            double minutes = 28 + (d * n) % 65;
            double energy = c["power"].toDouble() * minutes / 60.0;
            double amount = energy * c["price"].toDouble();

            q.prepare("INSERT INTO charging_order(user_id, charger_id, status, created_at, start_time, end_time, energy, amount) VALUES(?,?,2,?,?,?,?,?)");
            q.addBindValue(uid);
            q.addBindValue(c["id"]);
            q.addBindValue(end.addSecs(-static_cast<int>(minutes * 60)).toString("yyyy-MM-dd HH:mm:ss"));
            q.addBindValue(end.addSecs(-static_cast<int>(minutes * 60)).toString("yyyy-MM-dd HH:mm:ss"));
            q.addBindValue(end.toString("yyyy-MM-dd HH:mm:ss"));
            q.addBindValue(energy);
            q.addBindValue(amount);
            q.exec();

            q.prepare("UPDATE charger SET total_count=total_count+1, total_minutes=total_minutes+? WHERE id=?");
            q.addBindValue(minutes);
            q.addBindValue(c["id"]);
            q.exec();
        }
    }
    DatabaseManager::instance().commitTransaction();
}

// 生成辅助测试数据（充值日志、运维日志、负荷预测等）
void seedAuxiliaryData() {
    QSqlQuery q(DatabaseManager::instance().db());
    q.exec("SELECT (SELECT count(*) FROM recharge_log) + (SELECT count(*) FROM ops_log) + (SELECT count(*) FROM load_prediction)");
    if (q.next() && q.value(0).toInt()) return;

    const char* acts[] = {"上线", "下线", "故障", "重启", "空闲"};
    const int hours[] = {1, 6, 12, 24, 48, 72};

    DatabaseManager::instance().beginTransaction();
    for (int i = 2; i <= 12; ++i) {
        QString phone = QString("1800000%1").arg(i, 4, 10, QChar('0'));
        
        q.prepare("INSERT OR IGNORE INTO user(phone, nickname, balance, status, created_at) VALUES(?,?,?,?,?)");
        q.addBindValue(phone);
        q.addBindValue("种子用户" + QString::number(i));
        q.addBindValue((i * 37) % 500);
        q.addBindValue(1);
        q.addBindValue(QDateTime::currentDateTime().addSecs(-i * 3600).toString("yyyy-MM-dd HH:mm:ss"));
        q.exec();

        q.prepare("INSERT INTO recharge_log(user_id, amount, created_at) VALUES(?,?,?)");
        q.addBindValue(i % 12 + 1);
        q.addBindValue(20 + (i * 13) % 120);
        q.addBindValue(QDateTime::currentDateTime().addSecs(-i * 1800).toString("yyyy-MM-dd HH:mm:ss"));
        q.exec();

        q.prepare("INSERT INTO ops_log(charger_id, action, created_at) VALUES(?,?,?)");
        q.addBindValue(i * 3);
        q.addBindValue(acts[i % 5]);
        q.addBindValue(QDateTime::currentDateTime().addSecs(-i * 2700).toString("yyyy-MM-dd HH:mm:ss"));
        q.exec();
    }

    for (int s = 1; s <= 5; ++s) {
        for (int h : hours) {
            q.prepare("INSERT INTO load_prediction(station_id, target_time, predicted_energy, predicted_idle, is_peak) VALUES(?,?,?,?,?)");
            q.addBindValue(s);
            q.addBindValue(QDateTime::currentDateTime().addSecs(h * 3600).toString("yyyy-MM-dd HH:mm:ss"));
            q.addBindValue(20 + (s * h) % 50);
            q.addBindValue(3 + (s + h) % 8);
            q.addBindValue(h == 12 ? 1 : 0);
            q.exec();
        }
    }
    DatabaseManager::instance().commitTransaction();
}

} // namespace

// 获取数据库文件存储路径
QString PlatformService::databasePath() {
    auto p = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/NCS";
    QDir().mkpath(p);
    return p + "/charge_platform.db";
}

// 初始化数据库连接、创建数据表及初始化种子数据
bool PlatformService::initialize(QString *e) {
    if (!DatabaseManager::instance().open(databasePath())) {
        if (e) *e = "无法打开 SQLite 数据库";
        return false;
    }

    QSqlQuery q(DatabaseManager::instance().db());
    const QStringList ss = {
        "CREATE TABLE IF NOT EXISTS schema_version(version INTEGER)",
        "CREATE TABLE IF NOT EXISTS user(id INTEGER PRIMARY KEY AUTOINCREMENT, phone TEXT UNIQUE, nickname TEXT, avatar TEXT, balance REAL DEFAULT 0, status INTEGER DEFAULT 1, created_at TEXT)",
        "CREATE TABLE IF NOT EXISTS admin(id INTEGER PRIMARY KEY, account TEXT UNIQUE, password_hash TEXT, salt TEXT)",
        "CREATE TABLE IF NOT EXISTS station(id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT, address TEXT, longitude REAL, latitude REAL, price REAL)",
        "CREATE TABLE IF NOT EXISTS charger(id INTEGER PRIMARY KEY AUTOINCREMENT, station_id INTEGER, code TEXT UNIQUE, type TEXT, power REAL, status INTEGER DEFAULT 0, total_count INTEGER DEFAULT 0, total_minutes REAL DEFAULT 0)",
        "CREATE TABLE IF NOT EXISTS charging_order(id INTEGER PRIMARY KEY AUTOINCREMENT, user_id INTEGER, charger_id INTEGER, status INTEGER, created_at TEXT, start_time TEXT, end_time TEXT, energy REAL DEFAULT 0, amount REAL DEFAULT 0)",
        "CREATE TABLE IF NOT EXISTS recharge_log(id INTEGER PRIMARY KEY, user_id INTEGER, amount REAL, created_at TEXT)",
        "CREATE TABLE IF NOT EXISTS ops_log(id INTEGER PRIMARY KEY, charger_id INTEGER, action TEXT, created_at TEXT)",
        "CREATE TABLE IF NOT EXISTS load_prediction(id INTEGER PRIMARY KEY, station_id INTEGER, target_time TEXT, predicted_energy REAL, predicted_idle INTEGER, is_peak INTEGER)"
    };

    for (auto &s : ss) {
        if (!q.exec(s)) {
            if (e) *e = q.lastError().text();
            return false;
        }
    }

    q.exec("SELECT count(*) FROM station");
    q.next();
    if (q.value(0).toInt()) {
        seedDemoHistory();
        seedAuxiliaryData();
        return true;
    }

    // 初始化管理员账号
    q.prepare("INSERT INTO admin(account, password_hash, salt) VALUES('admin', ?, 'ncs-salt')");
    q.addBindValue(QCryptographicHash::hash("ncs-salt123456", QCryptographicHash::Sha256).toHex());
    q.exec();

    // 初始化预置充电站及电桩
    struct S { const char *n, *a; double lo, la, p; };
    S x[] = {
        {"人民广场充电站", "黄浦区人民大道", 121.4737, 31.2304, 1.25},
        {"陆家嘴新能源站", "浦东新区世纪大道", 121.5064, 31.2450, 1.35},
        {"徐家汇充电站", "徐汇区虹桥路", 121.4365, 31.1883, 1.18},
        {"五角场充电站", "杨浦区邯郸路", 121.5160, 31.3045, 1.20},
        {"虹桥枢纽充电站", "闵行区申贵路", 121.3200, 31.1960, 1.30}
    };

    DatabaseManager::instance().beginTransaction();
    for (int n = 0; n < 5; ++n) {
        q.prepare("INSERT INTO station(name, address, longitude, latitude, price) VALUES(?,?,?,?,?)");
        q.addBindValue(x[n].n);
        q.addBindValue(x[n].a);
        q.addBindValue(x[n].lo);
        q.addBindValue(x[n].la);
        q.addBindValue(x[n].p);
        q.exec();
        
        auto id = q.lastInsertId().toInt();
        for (int i = 1; i <= 12; i++) {
            q.prepare("INSERT INTO charger(station_id, code, type, power, status) VALUES(?,?,?,?,?)");
            q.addBindValue(id);
            q.addBindValue(QString("S%1-%2").arg(n + 1).arg(i, 2, 10, QChar('0')));
            q.addBindValue(i % 2 ? "快充" : "慢充");
            q.addBindValue(i % 2 ? 120 : 7);
            q.addBindValue(i == 12 ? 2 : 0); // 第12个桩设为故障状态用于演示
            q.exec();
        }
    }
    
    bool ok = DatabaseManager::instance().commitTransaction();
    if (ok) {
        seedDemoHistory();
        seedAuxiliaryData();
    }
    return ok;
}

// 用户登录或自动注册
User PlatformService::loginOrRegister(const QString &p, QString *e) {
    User u;
    if (!QRegularExpression("^1\\d{10}$").match(p).hasMatch()) {
        if (e) *e = "请输入正确的 11 位手机号";
        return u;
    }

    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare("SELECT * FROM user WHERE phone=?");
    q.addBindValue(p);
    q.exec();

    if (!q.next()) {
        q.prepare("INSERT INTO user(phone, nickname, created_at) VALUES(?,?,?)");
        q.addBindValue(p);
        q.addBindValue("用户" + p.right(4));
        q.addBindValue(now());
        q.exec();

        q.prepare("SELECT * FROM user WHERE phone=?");
        q.addBindValue(p);
        q.exec();
        q.next();
    }

    u = {
        q.value("id").toInt(),
        p,
        q.value("nickname").toString(),
        q.value("avatar").toString(),
        q.value("balance").toDouble(),
        q.value("status").toInt(),
        q.value("created_at").toString()
    };

    if (!u.status) {
        if (e) *e = "账号已被冻结，请联系客服";
        u = {};
    }
    return u;
}

// 管理员登录校验
bool PlatformService::adminLogin(const QString &a, const QString &p) {
    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare("SELECT password_hash, salt FROM admin WHERE account=?");
    q.addBindValue(a);
    return q.exec() && q.next() && 
           q.value(0).toByteArray() == QCryptographicHash::hash(q.value(1).toString().toUtf8() + p.toUtf8(), QCryptographicHash::Sha256).toHex();
}

// 获取充电站列表（带距离计算并按距离排序）
QVariantList PlatformService::stations(double la, double lo) {
    QVariantList l;
    QSqlQuery q(DatabaseManager::instance().db());
    q.exec("SELECT s.*, sum(case when c.status=0 then 1 else 0 end) idle, count(c.id) total FROM station s left join charger c on c.station_id=s.id GROUP BY s.id");
    
    while (q.next()) {
        auto m = map(q);
        m["distance"] = haversineKm(la, lo, m["latitude"].toDouble(), m["longitude"].toDouble());
        l << m;
    }

    std::sort(l.begin(), l.end(), [](auto &a, auto &b) {
        return a.toMap()["distance"].toDouble() < b.toMap()["distance"].toDouble();
    });
    return l;
}

// 获取指定电站详情
QVariantMap PlatformService::station(int id) {
    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare("SELECT s.*, count(c.id) total, sum(case when c.status=0 then 1 else 0 end) idle FROM station s LEFT JOIN charger c ON c.station_id=s.id WHERE s.id=? GROUP BY s.id");
    q.addBindValue(id);
    return q.exec() && q.next() ? map(q) : QVariantMap{};
}

// 获取电桩列表
QVariantList PlatformService::chargers(int s) {
    QVariantList l;
    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare("SELECT c.*, station.name station_name FROM charger c join station on station.id=c.station_id WHERE (?=0 or c.station_id=?) ORDER BY c.id");
    q.addBindValue(s);
    q.addBindValue(s);
    q.exec();
    
    while (q.next()) {
        l << map(q);
    }
    return l;
}

// 获取订单列表
QVariantList PlatformService::orders(int u) {
    QVariantList l;
    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare("SELECT o.*, station.name station_name, charger.code charger_code, station.price, charger.power FROM charging_order o join charger on charger.id=o.charger_id join station on station.id=charger.station_id WHERE (?=0 or o.user_id=?) ORDER BY o.id DESC");
    q.addBindValue(u);
    q.addBindValue(u);
    q.exec();
    
    while (q.next()) {
        l << map(q);
    }
    return l;
}

// 获取用户列表（支持手机号搜索）
QVariantList PlatformService::users(const QString &k) {
    QVariantList l;
    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare("SELECT * FROM user WHERE phone LIKE ? ORDER BY id DESC");
    q.addBindValue("%" + k + "%");
    q.exec();
    
    while (q.next()) {
        l << map(q);
    }
    return l;
}

// 获取每日营收统计
QVariantList PlatformService::revenueDays(int d) {
    QVariantList l;
    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare("SELECT substr(end_time, 1, 10) day, sum(amount) revenue, count(*) orders FROM charging_order WHERE status=2 AND end_time>=date('now', ?) GROUP BY day ORDER BY day");
    q.addBindValue("-" + QString::number(d) + " days");
    q.exec();
    
    while (q.next()) {
        l << map(q);
    }
    return l;
}

// 获取管理端统计指标
QVariantMap PlatformService::metrics() {
    QVariantMap m;
    QSqlQuery q(DatabaseManager::instance().db());
    
    q.exec("SELECT count(*), coalesce(sum(amount), 0) FROM charging_order WHERE status=2");
    q.next();
    m["orders"] = q.value(0);
    m["revenue"] = q.value(1);

    q.exec("SELECT count(*) FROM charger WHERE status<>2");
    q.next();
    m["online"] = q.value(0);

    q.exec("SELECT count(*) FROM user");
    q.next();
    m["users"] = q.value(0);

    return m;
}

// 用户余额充值
bool PlatformService::recharge(int id, double a, QString *e) {
    if (a <= 0 || a > 10000) {
        if (e) *e = "充值金额范围为 0.01 - 10000";
        return false;
    }
    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare("UPDATE user SET balance=balance+? WHERE id=?");
    q.addBindValue(a);
    q.addBindValue(id);
    return q.exec();
}

// 修改昵称
bool PlatformService::updateNickname(int id, const QString &n, QString *e) {
    if (n.trimmed().isEmpty() || n.size() > 20) {
        if (e) *e = "昵称不能为空且最多 20 字符";
        return false;
    }
    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare("UPDATE user SET nickname=? WHERE id=?");
    q.addBindValue(n.trimmed());
    q.addBindValue(id);
    return q.exec();
}

// 预约电桩（含未结算订单拦截与余额校验）
bool PlatformService::reserve(int u, int c, QString *e) {
    QSqlQuery q(DatabaseManager::instance().db());
    
    // 检查是否有未完成的订单 (status 0: 预约中, 1: 充电中)
    q.prepare("SELECT count(*) FROM charging_order WHERE user_id=? AND status in(0, 1)");
    q.addBindValue(u);
    q.exec();
    q.next();
    if (q.value(0).toInt()) {
        if (e) *e = "您有未完成的充电订单，请先结算";
        return false;
    }

    // 检查账户余额是否满足最低要求 (>= 5元)
    q.prepare("SELECT balance FROM user WHERE id=?");
    q.addBindValue(u);
    q.exec();
    q.next();
    if (q.value(0).toDouble() < 5) {
        if (e) *e = "余额不足，请先充值";
        return false;
    }

    DatabaseManager::instance().beginTransaction();
    
    // 更新电桩状态为占用 (status = 1)
    q.prepare("UPDATE charger SET status=1 WHERE id=? AND status=0");
    q.addBindValue(c);
    bool ok = q.exec() && q.numRowsAffected() == 1;

    if (ok) {
        // 创建预约订单 (status = 0)
        q.prepare("INSERT INTO charging_order(user_id, charger_id, status, created_at) VALUES(?,?,0,?)");
        q.addBindValue(u);
        q.addBindValue(c);
        q.addBindValue(now());
        ok = q.exec();
    }

    if (ok) {
        return DatabaseManager::instance().commitTransaction();
    }

    DatabaseManager::instance().rollbackTransaction();
    if (e) *e = "该电桩刚被占用，请重新选择";
    return false;
}

// 开始充电
bool PlatformService::start(int u, QString *e) {
    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare("UPDATE charging_order SET status=1, start_time=? WHERE user_id=? AND status=0");
    q.addBindValue(now());
    q.addBindValue(u);
    
    if (!q.exec() || q.numRowsAffected() != 1) {
        if (e) *e = "没有可开始的预约";
        return false;
    }
    return true;
}

// 结束充电并结算
bool PlatformService::settle(int u, QString *e) {
    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare("SELECT o.id, o.charger_id, station.price, charger.power, o.start_time FROM charging_order o join charger on charger.id=o.charger_id join station on station.id=charger.station_id WHERE o.user_id=? AND o.status=1");
    q.addBindValue(u);
    
    if (!q.exec() || !q.next()) {
        if (e) *e = "没有充电中的订单";
        return false;
    }

    int oid = q.value(0).toInt();
    int cid = q.value(1).toInt();
    int realSec = QDateTime::fromString(q.value(4).toString(), "yyyy-MM-dd HH:mm:ss").secsTo(QDateTime::currentDateTime());
    int simSec = realSec * 60; // 演示加速倍率
    double en = q.value(3).toDouble() * simSec / 3600.0;
    double am = en * q.value(2).toDouble();

    DatabaseManager::instance().beginTransaction();
    
    // 更新订单状态为已完成 (2)
    q.prepare("UPDATE charging_order SET status=2, end_time=?, energy=?, amount=? WHERE id=?");
    q.addBindValue(now());
    q.addBindValue(en);
    q.addBindValue(am);
    q.addBindValue(oid);
    bool ok = q.exec();

    // 释放电桩状态为闲置 (0)，更新使用次数与总分钟数
    q.prepare("UPDATE charger SET status=0, total_count=total_count+1, total_minutes=total_minutes+? WHERE id=?");
    q.addBindValue(simSec / 60.);
    q.addBindValue(cid);
    ok &= q.exec();

    // 扣减用户余额
    q.prepare("UPDATE user SET balance=max(0, balance-?) WHERE id=?");
    q.addBindValue(am);
    q.addBindValue(u);
    ok &= q.exec();

    return ok ? DatabaseManager::instance().commitTransaction() : (DatabaseManager::instance().rollbackTransaction(), false);
}

// 设置用户状态
bool PlatformService::setUserStatus(int i, int s) {
    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare("UPDATE user SET status=? WHERE id=?");
    q.addBindValue(s);
    q.addBindValue(i);
    return q.exec();
}

// 设置电桩状态
bool PlatformService::setChargerStatus(int i, int s) {
    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare("UPDATE charger SET status=? WHERE id=?");
    q.addBindValue(s);
    q.addBindValue(i);
    return q.exec();
}

// 取消预约
bool PlatformService::cancelReservation(int u, QString *e) {
    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare("SELECT charger_id FROM charging_order WHERE user_id=? AND status=0 ORDER BY id DESC LIMIT 1");
    q.addBindValue(u);
    
    if (!q.exec() || !q.next()) {
        if (e) *e = "没有可取消的预约";
        return false;
    }

    const int cid = q.value(0).toInt();
    DatabaseManager::instance().beginTransaction();

    q.prepare("UPDATE charging_order SET status=3, end_time=? WHERE user_id=? AND status=0");
    q.addBindValue(now());
    q.addBindValue(u);
    bool ok = q.exec();

    q.prepare("UPDATE charger SET status=0 WHERE id=?");
    q.addBindValue(cid);
    ok &= q.exec();

    return ok ? DatabaseManager::instance().commitTransaction() : (DatabaseManager::instance().rollbackTransaction(), false);
}

// 获取电桩总览统计
QVariantMap PlatformService::chargerOverview() {
    QVariantMap r;
    QSqlQuery q(DatabaseManager::instance().db());
    q.exec("SELECT count(*) total, sum(case when status=0 then 1 else 0 end) idle, sum(case when status=1 then 1 else 0 end) busy, sum(case when status=2 then 1 else 0 end) fault FROM charger");
    
    if (q.next()) {
        r["total"] = q.value("total");
        r["idle"] = q.value("idle");
        r["busy"] = q.value("busy");
        r["fault"] = q.value("fault");
        r["health"] = q.value("total").toInt() ? 100.0 * (q.value("total").toInt() - q.value("fault").toInt()) / q.value("total").toInt() : 0.0;
    }
    return r;
}

// 获取预测数据
QVariantList PlatformService::predictions() {
    QVariantList r;
    QSqlQuery q(DatabaseManager::instance().db());
    q.exec("SELECT p.*, s.name station_name FROM load_prediction p JOIN station s ON s.id=p.station_id ORDER BY p.target_time, p.station_id");
    while (q.next()) {
        r << map(q);
    }
    return r;
}

// 远程重启电桩
bool PlatformService::restartCharger(int id, QString *e) {
    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare("SELECT status FROM charger WHERE id=?");
    q.addBindValue(id);
    
    if (!q.exec() || !q.next()) {
        if (e) *e = "电桩不存在";
        return false;
    }
    if (q.value(0).toInt() == 1) {
        if (e) *e = "该电桩正在充电，不能远程重启";
        return false;
    }

    DatabaseManager::instance().beginTransaction();
    q.prepare("UPDATE charger SET status=0 WHERE id=?");
    q.addBindValue(id);
    bool ok = q.exec();

    q.prepare("INSERT INTO ops_log(charger_id, action, created_at) VALUES(?,?,?)");
    q.addBindValue(id);
    q.addBindValue("远程重启并恢复正常");
    q.addBindValue(now());
    ok &= q.exec();

    return ok ? DatabaseManager::instance().commitTransaction() : (DatabaseManager::instance().rollbackTransaction(), false);
}

// 保存充电站信息
bool PlatformService::saveStation(int id, const QString &name, const QString &addr, double lon, double lat, double price, QString *e) {
    if (name.trimmed().isEmpty() || lon < -180 || lon > 180 || lat < -90 || lat > 90 || price <= 0) {
        if (e) *e = "请检查站名、经纬度和单价";
        return false;
    }
    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare(id ? "UPDATE station SET name=?, address=?, longitude=?, latitude=?, price=? WHERE id=?" 
                 : "INSERT INTO station(name, address, longitude, latitude, price) VALUES(?,?,?,?,?)");
    q.addBindValue(name.trimmed());
    q.addBindValue(addr.trimmed());
    q.addBindValue(lon);
    q.addBindValue(lat);
    q.addBindValue(price);
    if (id) q.addBindValue(id);
    return q.exec();
}

// 删除充电站
bool PlatformService::deleteStation(int id, QString *e) {
    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare("SELECT count(*) FROM charger WHERE station_id=?");
    q.addBindValue(id);
    q.exec();
    q.next();
    
    if (q.value(0).toInt()) {
        if (e) *e = "该电站下仍有电桩，禁止删除";
        return false;
    }
    q.prepare("DELETE FROM station WHERE id=?");
    q.addBindValue(id);
    return q.exec();
}

// 保存电桩信息
bool PlatformService::saveCharger(int id, int stationId, const QString &code, const QString &type, double power, QString *e) {
    if (stationId <= 0 || code.trimmed().isEmpty() || power <= 0 || (type != "快充" && type != "慢充")) {
        if (e) *e = "请填写有效的电桩信息";
        return false;
    }
    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare(id ? "UPDATE charger SET station_id=?, code=?, type=?, power=? WHERE id=?" 
                 : "INSERT INTO charger(station_id, code, type, power, status) VALUES(?,?,?,?,0)");
    q.addBindValue(stationId);
    q.addBindValue(code.trimmed());
    q.addBindValue(type);
    q.addBindValue(power);
    if (id) q.addBindValue(id);

    if (!q.exec()) {
        if (e) *e = "编号已存在或保存失败";
        return false;
    }
    return true;
}

// 删除电桩
bool PlatformService::deleteCharger(int id, QString *e) {
    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare("DELETE FROM charger WHERE id=? AND status<>1");
    q.addBindValue(id);
    
    if (!q.exec() || q.numRowsAffected() != 1) {
        if (e) *e = "使用中的电桩禁止删除";
        return false;
    }
    return true;
}