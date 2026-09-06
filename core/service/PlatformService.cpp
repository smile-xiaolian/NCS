// 节选自 PlatformService.cpp 修改部分，完整逻辑如下：
#include "PlatformService.h"
#include "core/database/DatabaseManager.h"
#include "core/utils/Haversine.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QCryptographicHash>
#include <QDateTime>
#include <QRegularExpression>
#include <algorithm>

namespace { 
    QString now(){return QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");} 
    QVariantMap map(QSqlQuery&q){QVariantMap m;auto r=q.record();for(int i=0;i<r.count();++i)m[r.fieldName(i)]=q.value(i);return m;}
    
    void seedDemoHistory(){
        QSqlQuery q(DatabaseManager::instance().db());
        q.exec("SELECT count(*) FROM charging_order WHERE status=2");
        q.next();
        if(q.value(0).toInt())return;
        q.prepare("SELECT id FROM user WHERE phone=?");
        q.addBindValue("13800138000");
        q.exec();
        int uid=0;
        if(q.next())uid=q.value(0).toInt();
        else{
            q.prepare("INSERT INTO user(phone,nickname,balance,debt,status,created_at)VALUES(?,?,?,?,?,?)");
            q.addBindValue("13800138000");
            q.addBindValue("演示车主");
            q.addBindValue(10000.0);
            q.addBindValue(0.0);
            q.addBindValue(1);
            q.addBindValue(now());
            q.exec();
            uid=q.lastInsertId().toInt();
        }
        // ... 其余种子历史数据保持原样 ...
    }
    
    void seedAuxiliaryData(){
        QSqlQuery q(DatabaseManager::instance().db());
        q.exec("SELECT (SELECT count(*) FROM recharge_log)+(SELECT count(*) FROM ops_log)+(SELECT count(*) FROM load_prediction)");
        if(q.next()&&q.value(0).toInt())return;
        // ... 保持原样 ...
    }
}

constexpr int kSchemaVersion = 2; // 版本升级到 2

bool execSchemaScript(QString *error)
{
    QFile f(QStringLiteral(":/db/schema.sql"));
    if (!f.open(QFile::ReadOnly | QFile::Text))
    {
        if (error) *error = QStringLiteral("无法加载内置建表脚本 db/schema.sql");
        return false;
    }

    QSqlQuery q(DatabaseManager::instance().db());
    const QStringList statements = QString::fromUtf8(f.readAll()).split(';');

    for (const QString &stmt : statements)
    {
        QString cleaned;
        for (const QString &line : stmt.split('\n'))
        {
            const QString t = line.trimmed();
            if (t.isEmpty() || t.startsWith("--")) continue;
            cleaned += t + ' ';
        }
        if (cleaned.trimmed().isEmpty()) continue;
        if (!q.exec(cleaned))
        {
            if (error) *error = q.lastError().text();
            return false;
        }
    }
    return true;
}

int schemaVersion()
{
    QSqlQuery q(DatabaseManager::instance().db());
    q.exec(QStringLiteral("SELECT version FROM schema_version ORDER BY version DESC LIMIT 1"));
    return q.next() ? q.value(0).toInt() : 0;
}

bool setSchemaVersion(int version, QString *error)
{
    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare(QStringLiteral("INSERT INTO schema_version(version) VALUES(?)"));
    q.addBindValue(version);
    if (!q.exec())
    {
        if (error) *error = q.lastError().text();
        return false;
    }
    return true;
}

// 数据库初始化与版本迁移
bool PlatformService::initialize(QString *error)
{
    if (!DatabaseManager::instance().open(databasePath()))
    {
        if (error) *error = QStringLiteral("无法打开 SQLite 数据库");
        return false;
    }

    QSqlQuery q(DatabaseManager::instance().db());
    if (!q.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS schema_version(version INTEGER)")))
    {
        if (error) *error = q.lastError().text();
        return false;
    }

    const int version = schemaVersion();
    if (version < kSchemaVersion)
    {
        // 自动执行最新 schema.sql 脚本（利用 CREATE TABLE IF NOT EXISTS 以及通过ALTER兼容）
        q.exec("ALTER TABLE user ADD COLUMN debt REAL DEFAULT 0"); // 尝试兼容旧表加字段
        if (!execSchemaScript(error)) return false;
        
        // 更新版本号
        q.exec("DELETE FROM schema_version");
        if (!setSchemaVersion(kSchemaVersion, error)) return false;
    }

    // 检查种子数据
    q.exec(QStringLiteral("SELECT count(*) FROM station"));
    if (q.next() && q.value(0).toInt() > 0)
    {
        q.finish();
        seedDemoHistory();
        seedAuxiliaryData();
        return true;
    }
    q.finish();

    // 写入默认管理员与站点种子数据
    q.prepare(QStringLiteral("INSERT INTO admin(account,password_hash,salt)VALUES('admin',?,'ncs-salt')"));
    q.addBindValue(QCryptographicHash::hash("ncs-salt123456", QCryptographicHash::Sha256).toHex());
    q.exec();

    struct SeedStation { const char *name; const char *addr; double lng; double lat; double price; };
    const SeedStation seeds[] = {
        {"人民广场充电站", "黄浦区人民大道", 121.4737, 31.2304, 1.25},
        {"陆家嘴新能源站", "浦东新区世纪大道", 121.5064, 31.2450, 1.35},
        {"徐家汇充电站", "徐汇区虹桥路", 121.4365, 31.1883, 1.18},
        {"五角场充电站", "杨浦区邯郸路", 121.5160, 31.3045, 1.20},
        {"虹桥枢纽充电站", "闵行区申贵路", 121.3200, 31.1960, 1.30}
    };

    DatabaseManager::instance().beginTransaction();
    int stationNo = 0;
    for (const SeedStation &seed : seeds)
    {
        ++stationNo;
        q.prepare(QStringLiteral("INSERT INTO station(name,address,longitude,latitude,price)VALUES(?,?,?,?,?)"));
        q.addBindValue(QString::fromUtf8(seed.name));
        q.addBindValue(QString::fromUtf8(seed.addr));
        q.addBindValue(seed.lng);
        q.addBindValue(seed.lat);
        q.addBindValue(seed.price);
        q.exec();
        const int stationId = q.lastInsertId().toInt();

        for (int i = 1; i <= 12; ++i)
        {
            q.prepare(QStringLiteral("INSERT INTO charger(station_id,code,type,power,status)VALUES(?,?,?,?,?)"));
            q.addBindValue(stationId);
            q.addBindValue(QString("S%1-%2").arg(stationNo).arg(i, 2, 10, QChar('0')));
            q.addBindValue(i % 2 ? QStringLiteral("快充") : QStringLiteral("慢充"));
            q.addBindValue(i % 2 ? 120.0 : 7.0);
            q.addBindValue(i == 12 ? 2 : 0);
            q.exec();
        }
    }

    const bool ok = DatabaseManager::instance().commitTransaction();
    if (ok)
    {
        seedDemoHistory();
        seedAuxiliaryData();
    }
    return ok;
}

QString PlatformService::databasePath(){auto p=QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)+"/NCS";QDir().mkpath(p);return p+"/charge_platform.db";}

User PlatformService::loginOrRegister(const QString&p,QString*e){
    User u;
    if(!QRegularExpression("^1\\d{10}$").match(p).hasMatch()){if(e)*e="请输入正确的 11 位手机号";return u;}
    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare("SELECT * FROM user WHERE phone=?");
    q.addBindValue(p);
    q.exec();
    if(!q.next()){
        q.prepare("INSERT INTO user(phone,nickname,balance,debt,created_at)VALUES(?,?,0,0,?)");
        q.addBindValue(p);
        q.addBindValue("用户"+p.right(4));
        q.addBindValue(now());
        q.exec();
        q.prepare("SELECT * FROM user WHERE phone=?");
        q.addBindValue(p);
        q.exec();
        q.next();
    }
    u={q.value("id").toInt(),p,q.value("nickname").toString(),q.value("avatar").toString(),q.value("balance").toDouble(),q.value("debt").toDouble(),q.value("status").toInt(),q.value("created_at").toString()};
    if(!u.status){if(e)*e="账号已被冻结，请联系客服";u={};}
    return u;
}

bool PlatformService::adminLogin(const QString&a,const QString&p){
    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare("SELECT password_hash,salt FROM admin WHERE account=?");
    q.addBindValue(a);
    return q.exec()&&q.next()&&q.value(0).toByteArray()==QCryptographicHash::hash(q.value(1).toString().toUtf8()+p.toUtf8(),QCryptographicHash::Sha256).toHex();
}

QVariantList PlatformService::stations(double la,double lo){
    QVariantList l;
    QSqlQuery q(DatabaseManager::instance().db());
    q.exec("SELECT s.*,sum(case when c.status=0 then 1 else 0 end) idle,count(c.id) total FROM station s left join charger c on c.station_id=s.id GROUP BY s.id");
    while(q.next()){auto m=map(q);m["distance"]=haversineKm(la,lo,m["latitude"].toDouble(),m["longitude"].toDouble());l<<m;}
    std::sort(l.begin(),l.end(),[](auto&a,auto&b){return a.toMap()["distance"].toDouble()<b.toMap()["distance"].toDouble();});
    return l;
}

QVariantList PlatformService::chargers(int s){
    QVariantList l;
    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare("SELECT c.*,station.name station_name FROM charger c join station on station.id=c.station_id WHERE (?=0 or c.station_id=?) ORDER BY c.id");
    q.addBindValue(s);q.addBindValue(s);q.exec();
    while(q.next())l<<map(q);
    return l;
} 

QVariantList PlatformService::orders(int u){
    QVariantList l;
    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare("SELECT o.*,station.name station_name,charger.code charger_code,station.price,charger.power FROM charging_order o join charger on charger.id=o.charger_id join station on station.id=charger.station_id WHERE (?=0 or o.user_id=?) ORDER BY o.id DESC");
    q.addBindValue(u);q.addBindValue(u);q.exec();
    while(q.next())l<<map(q);
    return l;
} 

QVariantList PlatformService::users(const QString&k){
    QVariantList l;
    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare("SELECT * FROM user WHERE phone LIKE ? ORDER BY id DESC");
    q.addBindValue("%"+k+"%");q.exec();
    while(q.next())l<<map(q);
    return l;
}

QVariantList PlatformService::revenueDays(int d){
    QVariantList l;
    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare("SELECT substr(end_time,1,10) day,sum(amount) revenue,count(*) orders FROM charging_order WHERE status=2 AND end_time>=date('now',?) GROUP BY day ORDER BY day");
    q.addBindValue("-"+QString::number(d)+" days");q.exec();
    while(q.next())l<<map(q);
    return l;
} 

QVariantMap PlatformService::metrics(){
    QVariantMap m;
    QSqlQuery q(DatabaseManager::instance().db());
    q.exec("SELECT count(*),coalesce(sum(amount),0) FROM charging_order WHERE status=2");
    q.next();m["orders"]=q.value(0);m["revenue"]=q.value(1);
    q.exec("SELECT count(*) FROM charger WHERE status<>2");
    q.next();m["online"]=q.value(0);
    q.exec("SELECT count(*) FROM user");
    q.next();m["users"]=q.value(0);
    return m;
}

// 充值逻辑：优先偿还欠费，剩余进入余额
bool PlatformService::recharge(int id,double a,QString*e){
    if(a<=0||a>10000){if(e)*e="充值金额范围为 0.01 - 10000";return false;}
    QSqlQuery q(DatabaseManager::instance().db());
    
    // 获取当前用户欠费
    q.prepare("SELECT debt, balance FROM user WHERE id=?");
    q.addBindValue(id);
    if(!q.exec()||!q.next()) { if(e)*e="用户不存在"; return false; }
    double currentDebt = q.value(0).toDouble();
    double currentBalance = q.value(1).toDouble();

    double newDebt = 0.0;
    double newBalance = currentBalance;

    if (currentDebt > 0) {
        if (a >= currentDebt) {
            newDebt = 0.0;
            newBalance += (a - currentDebt);
        } else {
            newDebt = currentDebt - a;
        }
    } else {
        newBalance += a;
    }

    q.prepare("UPDATE user SET balance=?, debt=? WHERE id=?");
    q.addBindValue(newBalance);
    q.addBindValue(newDebt);
    q.addBindValue(id);
    bool ok = q.exec();
    if(ok) {
        q.prepare("INSERT INTO recharge_log(user_id, amount, created_at) VALUES(?,?,?)");
        q.addBindValue(id);
        q.addBindValue(a);
        q.addBindValue(now());
        q.exec();
    }
    return ok;
} 

bool PlatformService::updateNickname(int id,const QString&n,QString*e){
    if(n.trimmed().isEmpty()||n.size()>20){if(e)*e="昵称不能为空且最多 20 字符";return false;}
    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare("UPDATE user SET nickname=? WHERE id=?");
    q.addBindValue(n.trimmed());q.addBindValue(id);
    return q.exec();
}

// 预约逻辑：检查是否存在欠费，若欠费拦截并要求先补缴
bool PlatformService::reserve(int u,int c,QString*e){
    QSqlQuery q(DatabaseManager::instance().db());
    
    // 检查欠费情况
    q.prepare("SELECT debt, balance FROM user WHERE id=?");
    q.addBindValue(u);
    q.exec();
    if(q.next()){
        double debt = q.value(0).toDouble();
        if(debt > 0){
            if(e) *e = QString("您有历史欠费 ¥ %1 未结清，请先在个人中心充值补缴后才能预约充电！").arg(debt, 0, 'f', 2);
            return false;
        }
    }

    q.prepare("SELECT count(*) FROM charging_order WHERE user_id=? AND status in(0,1)");
    q.addBindValue(u);q.exec();q.next();
    if(q.value(0).toInt()){if(e)*e="您有未完成的充电订单，请先结算";return false;}
    
    q.prepare("SELECT balance FROM user WHERE id=?");
    q.addBindValue(u);q.exec();q.next();
    if(q.value(0).toDouble()<5){if(e)*e="余额不足（低于起充金额5元），请先充值";return false;}

    DatabaseManager::instance().beginTransaction();
    q.prepare("UPDATE charger SET status=1 WHERE id=? AND status=0");
    q.addBindValue(c);
    bool ok=q.exec()&&q.numRowsAffected()==1;
    if(ok){
        q.prepare("INSERT INTO charging_order(user_id,charger_id,status,created_at)VALUES(?,?,0,?)");
        q.addBindValue(u);q.addBindValue(c);q.addBindValue(now());
        ok=q.exec();
    }
    if(ok)return DatabaseManager::instance().commitTransaction();
    DatabaseManager::instance().rollbackTransaction();
    if(e)*e="该电桩刚被占用，请重新选择";
    return false;
}

bool PlatformService::start(int u,QString*e){
    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare("UPDATE charging_order SET status=1,start_time=? WHERE user_id=? AND status=0");
    q.addBindValue(now());q.addBindValue(u);
    if(!q.exec()||q.numRowsAffected()!=1){if(e)*e="没有可开始的预约";return false;}
    return true;
} 

// 结算逻辑：余额不足时扣至0并记录到 debt 中
bool PlatformService::settle(int u,QString*e){
    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare("SELECT o.id,o.charger_id,station.price,charger.power,o.start_time FROM charging_order o join charger on charger.id=o.charger_id join station on station.id=charger.station_id WHERE o.user_id=? AND o.status=1");
    q.addBindValue(u);
    if(!q.exec()||!q.next()){if(e)*e="没有充电中的订单";return false;}
    
    int oid=q.value(0).toInt(),cid=q.value(1).toInt();
    int realSec=QDateTime::fromString(q.value(4).toString(),"yyyy-MM-dd HH:mm:ss").secsTo(QDateTime::currentDateTime()),simSec=realSec*60;
    double en=q.value(3).toDouble()*simSec/3600.0,am=en*q.value(2).toDouble();

    // 获取当前用户余额
    q.prepare("SELECT balance, debt FROM user WHERE id=?");
    q.addBindValue(u);
    q.exec();
    q.next();
    double currentBalance = q.value(0).toDouble();
    double currentDebt = q.value(1).toDouble();

    double newBalance = 0.0;
    double addedDebt = 0.0;

    if (currentBalance >= am) {
        newBalance = currentBalance - am;
    } else {
        newBalance = 0.0;
        addedDebt = am - currentBalance; // 扣至0后记录欠费金额
    }
    double totalDebt = currentDebt + addedDebt;

    DatabaseManager::instance().beginTransaction();
    q.prepare("UPDATE charging_order SET status=2,end_time=?,energy=?,amount=? WHERE id=?");
    q.addBindValue(now());q.addBindValue(en);q.addBindValue(am);q.addBindValue(oid);
    bool ok=q.exec();

    q.prepare("UPDATE charger SET status=0,total_count=total_count+1,total_minutes=total_minutes+? WHERE id=?");
    q.addBindValue(simSec/60.);q.addBindValue(cid);
    ok&=q.exec();

    q.prepare("UPDATE user SET balance=?, debt=? WHERE id=?");
    q.addBindValue(newBalance);
    q.addBindValue(totalDebt);
    q.addBindValue(u);
    ok&=q.exec();

    return ok?DatabaseManager::instance().commitTransaction():(DatabaseManager::instance().rollbackTransaction(),false);
} 

bool PlatformService::setUserStatus(int i,int s){
    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare("UPDATE user SET status=? WHERE id=?");
    q.addBindValue(s);q.addBindValue(i);
    return q.exec();
}

bool PlatformService::setChargerStatus(int i,int s){
    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare("UPDATE charger SET status=? WHERE id=?");
    q.addBindValue(s);q.addBindValue(i);
    return q.exec();
}

bool PlatformService::cancelReservation(int u,QString*e){
    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare("SELECT charger_id FROM charging_order WHERE user_id=? AND status=0 ORDER BY id DESC LIMIT 1");
    q.addBindValue(u);
    if(!q.exec()||!q.next()){if(e)*e="没有可取消的预约";return false;}
    const int cid=q.value(0).toInt();
    DatabaseManager::instance().beginTransaction();
    q.prepare("UPDATE charging_order SET status=3,end_time=? WHERE user_id=? AND status=0");
    q.addBindValue(now());q.addBindValue(u);
    bool ok=q.exec();
    q.prepare("UPDATE charger SET status=0 WHERE id=?");
    q.addBindValue(cid);
    ok&=q.exec();
    return ok?DatabaseManager::instance().commitTransaction():(DatabaseManager::instance().rollbackTransaction(),false);
}

QVariantMap PlatformService::station(int id){
    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare("SELECT s.*,count(c.id) total,sum(case when c.status=0 then 1 else 0 end) idle FROM station s LEFT JOIN charger c ON c.station_id=s.id WHERE s.id=? GROUP BY s.id");
    q.addBindValue(id);
    return q.exec()&&q.next()?map(q):QVariantMap{};
}

QVariantMap PlatformService::chargerOverview(){
    QVariantMap r;
    QSqlQuery q(DatabaseManager::instance().db());
    q.exec("SELECT count(*) total,sum(case when status=0 then 1 else 0 end) idle,sum(case when status=1 then 1 else 0 end) busy,sum(case when status=2 then 1 else 0 end) fault FROM charger");
    if(q.next()){
        r["total"]=q.value("total");r["idle"]=q.value("idle");r["busy"]=q.value("busy");r["fault"]=q.value("fault");
        r["health"]=q.value("total").toInt()?100.0*(q.value("total").toInt()-q.value("fault").toInt())/q.value("total").toInt():0.0;
    }
    return r;
}

QVariantList PlatformService::predictions(){
    QVariantList r;
    QSqlQuery q(DatabaseManager::instance().db());
    q.exec("SELECT p.*,s.name station_name FROM load_prediction p JOIN station s ON s.id=p.station_id ORDER BY p.target_time,p.station_id");
    while(q.next())r<<map(q);
    return r;
}

bool PlatformService::restartCharger(int id,QString*e){
    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare("SELECT status FROM charger WHERE id=?");
    q.addBindValue(id);
    if(!q.exec()||!q.next()){if(e)*e="电桩不存在";return false;}
    if(q.value(0).toInt()==1){if(e)*e="该电桩正在充电，不能远程重启";return false;}
    DatabaseManager::instance().beginTransaction();
    q.prepare("UPDATE charger SET status=0 WHERE id=?");
    q.addBindValue(id);
    bool ok=q.exec();
    q.prepare("INSERT INTO ops_log(charger_id,action,created_at) VALUES(?,?,?)");
    q.addBindValue(id);q.addBindValue("远程重启并恢复正常");q.addBindValue(now());
    ok&=q.exec();
    return ok?DatabaseManager::instance().commitTransaction():(DatabaseManager::instance().rollbackTransaction(),false);
}

bool PlatformService::saveStation(int id,const QString&name,const QString&addr,double lon,double lat,double price,QString*e){
    if(name.trimmed().isEmpty()||lon < -180||lon>180||lat<-90||lat>90||price<=0){if(e)*e="请检查站名、经纬度和单价";return false;}
    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare(id?"UPDATE station SET name=?,address=?,longitude=?,latitude=?,price=? WHERE id=?":"INSERT INTO station(name,address,longitude,latitude,price) VALUES(?,?,?,?,?)");
    q.addBindValue(name.trimmed());q.addBindValue(addr.trimmed());q.addBindValue(lon);q.addBindValue(lat);q.addBindValue(price);
    if(id)q.addBindValue(id);
    return q.exec();
}

bool PlatformService::deleteStation(int id,QString*e){
    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare("SELECT count(*) FROM charger WHERE station_id=?");
    q.addBindValue(id);q.exec();q.next();
    if(q.value(0).toInt()){if(e)*e="该电站下仍有电桩，禁止删除";return false;}
    q.prepare("DELETE FROM station WHERE id=?");
    q.addBindValue(id);
    return q.exec();
}

bool PlatformService::saveCharger(int id,int stationId,const QString&code,const QString&type,double power,QString*e){
    if(stationId<=0||code.trimmed().isEmpty()||power<=0||(type!="快充"&&type!="慢充")){if(e)*e="请填写有效的电桩信息";return false;}
    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare(id?"UPDATE charger SET station_id=?,code=?,type=?,power=? WHERE id=?":"INSERT INTO charger(station_id,code,type,power,status) VALUES(?,?,?,?,0)");
    q.addBindValue(stationId);q.addBindValue(code.trimmed());q.addBindValue(type);q.addBindValue(power);
    if(id)q.addBindValue(id);
    if(!q.exec()){if(e)*e="编号已存在或保存失败";return false;}
    return true;
}

bool PlatformService::deleteCharger(int id,QString*e){
    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare("DELETE FROM charger WHERE id=? AND status<>1");
    q.addBindValue(id);
    if(!q.exec()||q.numRowsAffected()!=1){if(e)*e="使用中的电桩禁止删除";return false;}
    return true;
}