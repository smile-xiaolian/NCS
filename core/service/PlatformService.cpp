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
namespace { QString now(){return QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");} QVariantMap map(QSqlQuery&q){QVariantMap m;auto r=q.record();for(int i=0;i<r.count();++i)m[r.fieldName(i)]=q.value(i);return m;}
void seedDemoHistory(){QSqlQuery q(DatabaseManager::instance().db());q.exec("SELECT count(*) FROM charging_order WHERE status=2");q.next();if(q.value(0).toInt())return;q.prepare("SELECT id FROM user WHERE phone=?");q.addBindValue("13800138000");q.exec();int uid=0;if(q.next())uid=q.value(0).toInt();else{q.prepare("INSERT INTO user(phone,nickname,balance,status,created_at)VALUES(?,?,?,?,?)");q.addBindValue("13800138000");q.addBindValue("演示车主");q.addBindValue(10000.0);q.addBindValue(1);q.addBindValue(now());q.exec();uid=q.lastInsertId().toInt();}q.exec("SELECT c.id,c.power,s.price FROM charger c JOIN station s ON s.id=c.station_id WHERE c.status<>2 ORDER BY c.id");QVariantList cs;while(q.next())cs<<map(q);if(cs.isEmpty())return;DatabaseManager::instance().beginTransaction();for(int d=30;d>0;--d){for(int n=0;n<12;++n){auto c=cs[(d+n)%cs.size()].toMap();auto end=QDateTime::currentDateTime().addDays(-d).addSecs(n*3600);double minutes=28+(d*n)%65,energy=c["power"].toDouble()*minutes/60.0,amount=energy*c["price"].toDouble();q.prepare("INSERT INTO charging_order(user_id,charger_id,status,created_at,start_time,end_time,energy,amount)VALUES(?,?,2,?,?,?,?,?)");q.addBindValue(uid);q.addBindValue(c["id"]);q.addBindValue(end.addSecs(-static_cast<int>(minutes*60)).toString("yyyy-MM-dd HH:mm:ss"));q.addBindValue(end.addSecs(-static_cast<int>(minutes*60)).toString("yyyy-MM-dd HH:mm:ss"));q.addBindValue(end.toString("yyyy-MM-dd HH:mm:ss"));q.addBindValue(energy);q.addBindValue(amount);q.exec();q.prepare("UPDATE charger SET total_count=total_count+1,total_minutes=total_minutes+? WHERE id=?");q.addBindValue(minutes);q.addBindValue(c["id"]);q.exec();}}DatabaseManager::instance().commitTransaction();} }
void seedAuxiliaryData(){QSqlQuery q(DatabaseManager::instance().db());q.exec("SELECT (SELECT count(*) FROM recharge_log)+(SELECT count(*) FROM ops_log)+(SELECT count(*) FROM load_prediction)");if(q.next()&&q.value(0).toInt())return;const char* acts[]={"上线","下线","故障","重启","空闲"};const int hours[]={1,6,12,24,48,72};DatabaseManager::instance().beginTransaction();for(int i=2;i<=12;++i){QString phone=QString("1800000%1").arg(i,4,10,QChar('0'));q.prepare("INSERT OR IGNORE INTO user(phone,nickname,balance,status,created_at)VALUES(?,?,?,?,?)");q.addBindValue(phone);q.addBindValue("种子用户"+QString::number(i));q.addBindValue((i*37)%500);q.addBindValue(1);q.addBindValue(QDateTime::currentDateTime().addSecs(-i*3600).toString("yyyy-MM-dd HH:mm:ss"));q.exec();q.prepare("INSERT INTO recharge_log(user_id,amount,created_at)VALUES(?,?,?)");q.addBindValue(i%12+1);q.addBindValue(20+(i*13)%120);q.addBindValue(QDateTime::currentDateTime().addSecs(-i*1800).toString("yyyy-MM-dd HH:mm:ss"));q.exec();q.prepare("INSERT INTO ops_log(charger_id,action,created_at)VALUES(?,?,?)");q.addBindValue(i*3);q.addBindValue(acts[i%5]);q.addBindValue(QDateTime::currentDateTime().addSecs(-i*2700).toString("yyyy-MM-dd HH:mm:ss"));q.exec();}for(int s=1;s<=5;++s){for(int h:hours){q.prepare("INSERT INTO load_prediction(station_id,target_time,predicted_energy,predicted_idle,is_peak)VALUES(?,?,?,?,?)");q.addBindValue(s);q.addBindValue(QDateTime::currentDateTime().addSecs(h*3600).toString("yyyy-MM-dd HH:mm:ss"));q.addBindValue(20+(s*h)%50);q.addBindValue(3+(s+h)%8);q.addBindValue(h==12?1:0);q.exec();}}DatabaseManager::instance().commitTransaction();}
// ---- 数据库版本与建表脚本管理 (UC-D-01 / UC-D-02 / UC-D-03) ----

// 当前数据库结构版本，与 db/schema.sql 保持一致
constexpr int kSchemaVersion = 1;

// 执行内嵌的 db/schema.sql 建表脚本（Qt 资源，幂等）
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
        // 去掉 -- 注释行后拼接，空语句跳过
        QString cleaned;
        for (const QString &line : stmt.split('\n'))
        {
            const QString t = line.trimmed();
            if (t.isEmpty() || t.startsWith("--"))
                continue;
            cleaned += t + ' ';
        }

        if (cleaned.trimmed().isEmpty())
            continue;

        if (!q.exec(cleaned))
        {
            if (error) *error = q.lastError().text();
            return false;
        }
    }
    return true;
}

// 读取当前数据库版本，无记录返回 0
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

// v0 -> v1 迁移：SQLite 无法直接给已有表添加外键，需重建相关表
bool migrateV0ToV1(QString *error)
{
    struct Step { const char *name; const char *ddl; const char *cols; };
    const Step steps[] = {
        {
            "charger",
            "CREATE TABLE charger(id INTEGER PRIMARY KEY AUTOINCREMENT,station_id INTEGER NOT NULL,"
            "code TEXT UNIQUE,type TEXT,power REAL,status INTEGER DEFAULT 0,"
            "total_count INTEGER DEFAULT 0,total_minutes REAL DEFAULT 0,"
            "CONSTRAINT fk_charger_station FOREIGN KEY(station_id) REFERENCES station(id) ON DELETE RESTRICT)",
            "id,station_id,code,type,power,status,total_count,total_minutes"
        },
        {
            "charging_order",
            "CREATE TABLE charging_order(id INTEGER PRIMARY KEY AUTOINCREMENT,user_id INTEGER NOT NULL,"
            "charger_id INTEGER NOT NULL,status INTEGER,created_at TEXT,start_time TEXT,end_time TEXT,"
            "energy REAL DEFAULT 0,amount REAL DEFAULT 0,"
            "CONSTRAINT fk_order_user FOREIGN KEY(user_id) REFERENCES user(id) ON DELETE RESTRICT,"
            "CONSTRAINT fk_order_charger FOREIGN KEY(charger_id) REFERENCES charger(id) ON DELETE CASCADE)",
            "id,user_id,charger_id,status,created_at,start_time,end_time,energy,amount"
        },
        {
            "recharge_log",
            "CREATE TABLE recharge_log(id INTEGER PRIMARY KEY,user_id INTEGER,amount REAL,created_at TEXT,"
            "CONSTRAINT fk_recharge_user FOREIGN KEY(user_id) REFERENCES user(id) ON DELETE RESTRICT)",
            "id,user_id,amount,created_at"
        },
        {
            "ops_log",
            "CREATE TABLE ops_log(id INTEGER PRIMARY KEY,charger_id INTEGER,action TEXT,created_at TEXT,"
            "CONSTRAINT fk_ops_charger FOREIGN KEY(charger_id) REFERENCES charger(id) ON DELETE CASCADE)",
            "id,charger_id,action,created_at"
        },
        {
            "load_prediction",
            "CREATE TABLE load_prediction(id INTEGER PRIMARY KEY,station_id INTEGER,target_time TEXT,"
            "predicted_energy REAL,predicted_idle INTEGER,is_peak INTEGER,"
            "CONSTRAINT fk_pred_station FOREIGN KEY(station_id) REFERENCES station(id) ON DELETE CASCADE)",
            "id,station_id,target_time,predicted_energy,predicted_idle,is_peak"
        }
    };

    QSqlQuery q(DatabaseManager::instance().db());

    // PRAGMA foreign_keys 不允许在事务内修改，须在开启事务前关闭
    q.exec(QStringLiteral("PRAGMA foreign_keys = OFF"));
    DatabaseManager::instance().beginTransaction();

    for (const Step &step : steps)
    {
        const QString oldName = QString::fromLatin1(step.name) + QStringLiteral("_v0");

        if (!q.exec(QStringLiteral("ALTER TABLE %1 RENAME TO %2").arg(QLatin1String(step.name), oldName))
            || !q.exec(QLatin1String(step.ddl))
            || !q.exec(QStringLiteral("INSERT INTO %1(%2) SELECT %2 FROM %3")
                           .arg(QLatin1String(step.name), QLatin1String(step.cols), oldName))
            || !q.exec(QStringLiteral("DROP TABLE %1").arg(oldName)))
        {
            const QString msg = q.lastError().text();
            qDebug() << "migrateV0ToV1 步骤失败:" << step.name << q.lastError();
            DatabaseManager::instance().rollbackTransaction();
            q.exec(QStringLiteral("PRAGMA foreign_keys = ON"));
            if (error) *error = msg;
            return false;
        }
    }

    if (!DatabaseManager::instance().commitTransaction())
    {
        DatabaseManager::instance().rollbackTransaction();
        q.exec(QStringLiteral("PRAGMA foreign_keys = ON"));
        if (error) *error = QStringLiteral("数据库迁移提交失败");
        return false;
    }

    q.exec(QStringLiteral("PRAGMA foreign_keys = ON"));
    return true;
}

QString PlatformService::databasePath(){auto p=QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)+"/NCS";QDir().mkpath(p);return p+"/charge_platform.db";}
bool PlatformService::initialize(QString *error)
{
    if (!DatabaseManager::instance().open(databasePath()))
    {
        if (error) *error = QStringLiteral("无法打开 SQLite 数据库");
        return false;
    }

    QSqlQuery q(DatabaseManager::instance().db());

    // 版本表最先创建，用于区分全新数据库与旧版数据库 (UC-D-02)
    if (!q.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS schema_version(version INTEGER)")))
    {
        if (error) *error = q.lastError().text();
        return false;
    }

    const int version = schemaVersion();

    if (version == 0)
    {
        // 判断是否为全新数据库（尚无任何业务表）
        q.exec(QStringLiteral("SELECT count(*) FROM sqlite_master WHERE type='table' AND name IN "
                              "('user','admin','station','charger','charging_order','recharge_log','ops_log','load_prediction')"));
        const bool fresh = q.next() && q.value(0).toInt() == 0;
        q.finish();  // 结束游标，否则同连接上后续的 DDL 会报 table is locked

        if (fresh)
        {
            // 全新数据库：执行建表脚本并写入版本号
            if (!execSchemaScript(error) || !setSchemaVersion(kSchemaVersion, error))
                return false;
        }
        else
        {
            // 旧版数据库：迁移到 v1，为既有表补充外键约束
            if (!migrateV0ToV1(error) || !setSchemaVersion(kSchemaVersion, error))
                return false;
        }
    }
    else if (version == kSchemaVersion)
    {
        // 版本一致：幂等执行建表脚本（IF NOT EXISTS 自愈）
        if (!execSchemaScript(error))
            return false;
    }
    else
    {
        // 数据库版本高于程序支持的版本
        if (error) *error = QStringLiteral("数据库版本 %1 高于程序支持的版本 %2")
                                .arg(version).arg(kSchemaVersion);
        return false;
    }

    // 站点为空说明是全新库，执行全量种子数据播种；否则仅补充演示数据 (UC-D-02)
    q.exec(QStringLiteral("SELECT count(*) FROM station"));
    if (q.next() && q.value(0).toInt() > 0)
    {
        q.finish();
        seedDemoHistory();
        seedAuxiliaryData();
        return true;
    }
    q.finish();

    // 种子数据：管理员账号 (admin / 123456)
    q.prepare(QStringLiteral("INSERT INTO admin(account,password_hash,salt)VALUES('admin',?,'ncs-salt')"));
    q.addBindValue(QCryptographicHash::hash("ncs-salt123456", QCryptographicHash::Sha256).toHex());
    q.exec();    // 种子数据：5 个电站、每站 12 台电桩（快慢充混合，每站 1 台故障） (UC-D-02)
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
User PlatformService::loginOrRegister(const QString&p,QString*e){User u;if(!QRegularExpression("^1\\d{10}$").match(p).hasMatch()){if(e)*e="请输入正确的 11 位手机号";return u;}QSqlQuery q(DatabaseManager::instance().db());q.prepare("SELECT * FROM user WHERE phone=?");q.addBindValue(p);q.exec();if(!q.next()){q.prepare("INSERT INTO user(phone,nickname,created_at)VALUES(?,?,?)");q.addBindValue(p);q.addBindValue("用户"+p.right(4));q.addBindValue(now());q.exec();q.prepare("SELECT * FROM user WHERE phone=?");q.addBindValue(p);q.exec();q.next();}u={q.value("id").toInt(),p,q.value("nickname").toString(),q.value("avatar").toString(),q.value("balance").toDouble(),q.value("status").toInt(),q.value("created_at").toString()};if(!u.status){if(e)*e="账号已被冻结，请联系客服";u={};}return u;}
bool PlatformService::adminLogin(const QString&a,const QString&p){QSqlQuery q(DatabaseManager::instance().db());q.prepare("SELECT password_hash,salt FROM admin WHERE account=?");q.addBindValue(a);return q.exec()&&q.next()&&q.value(0).toByteArray()==QCryptographicHash::hash(q.value(1).toString().toUtf8()+p.toUtf8(),QCryptographicHash::Sha256).toHex();}
QVariantList PlatformService::stations(double la,double lo){QVariantList l;QSqlQuery q(DatabaseManager::instance().db());q.exec("SELECT s.*,sum(case when c.status=0 then 1 else 0 end) idle,count(c.id) total FROM station s left join charger c on c.station_id=s.id GROUP BY s.id");while(q.next()){auto m=map(q);m["distance"]=haversineKm(la,lo,m["latitude"].toDouble(),m["longitude"].toDouble());l<<m;}std::sort(l.begin(),l.end(),[](auto&a,auto&b){return a.toMap()["distance"].toDouble()<b.toMap()["distance"].toDouble();});return l;}
QVariantList PlatformService::chargers(int s){QVariantList l;QSqlQuery q(DatabaseManager::instance().db());q.prepare("SELECT c.*,station.name station_name FROM charger c join station on station.id=c.station_id WHERE (?=0 or c.station_id=?) ORDER BY c.id");q.addBindValue(s);q.addBindValue(s);q.exec();while(q.next())l<<map(q);return l;} QVariantList PlatformService::orders(int u){QVariantList l;QSqlQuery q(DatabaseManager::instance().db());q.prepare("SELECT o.*,station.name station_name,charger.code charger_code,station.price,charger.power FROM charging_order o join charger on charger.id=o.charger_id join station on station.id=charger.station_id WHERE (?=0 or o.user_id=?) ORDER BY o.id DESC");q.addBindValue(u);q.addBindValue(u);q.exec();while(q.next())l<<map(q);return l;} QVariantList PlatformService::users(const QString&k){QVariantList l;QSqlQuery q(DatabaseManager::instance().db());q.prepare("SELECT * FROM user WHERE phone LIKE ? ORDER BY id DESC");q.addBindValue("%"+k+"%");q.exec();while(q.next())l<<map(q);return l;}
QVariantList PlatformService::revenueDays(int d){QVariantList l;QSqlQuery q(DatabaseManager::instance().db());q.prepare("SELECT substr(end_time,1,10) day,sum(amount) revenue,count(*) orders FROM charging_order WHERE status=2 AND end_time>=date('now',?) GROUP BY day ORDER BY day");q.addBindValue("-"+QString::number(d)+" days");q.exec();while(q.next())l<<map(q);return l;} QVariantMap PlatformService::metrics(){QVariantMap m;QSqlQuery q(DatabaseManager::instance().db());q.exec("SELECT count(*),coalesce(sum(amount),0) FROM charging_order WHERE status=2");q.next();m["orders"]=q.value(0);m["revenue"]=q.value(1);q.exec("SELECT count(*) FROM charger WHERE status<>2");q.next();m["online"]=q.value(0);q.exec("SELECT count(*) FROM user");q.next();m["users"]=q.value(0);return m;}
bool PlatformService::recharge(int id,double a,QString*e){if(a<=0||a>10000){if(e)*e="充值金额范围为 0.01 - 10000";return false;}QSqlQuery q(DatabaseManager::instance().db());q.prepare("UPDATE user SET balance=balance+? WHERE id=?");q.addBindValue(a);q.addBindValue(id);return q.exec();} bool PlatformService::updateNickname(int id,const QString&n,QString*e){if(n.trimmed().isEmpty()||n.size()>20){if(e)*e="昵称不能为空且最多 20 字符";return false;}QSqlQuery q(DatabaseManager::instance().db());q.prepare("UPDATE user SET nickname=? WHERE id=?");q.addBindValue(n.trimmed());q.addBindValue(id);return q.exec();}
bool PlatformService::reserve(int u,int c,QString*e){QSqlQuery q(DatabaseManager::instance().db());q.prepare("SELECT count(*) FROM charging_order WHERE user_id=? AND status in(0,1)");q.addBindValue(u);q.exec();q.next();if(q.value(0).toInt()){if(e)*e="您有未完成的充电订单，请先结算";return false;}q.prepare("SELECT balance FROM user WHERE id=?");q.addBindValue(u);q.exec();q.next();if(q.value(0).toDouble()<5){if(e)*e="余额不足，请先充值";return false;}DatabaseManager::instance().beginTransaction();q.prepare("UPDATE charger SET status=1 WHERE id=? AND status=0");q.addBindValue(c);bool ok=q.exec()&&q.numRowsAffected()==1;if(ok){q.prepare("INSERT INTO charging_order(user_id,charger_id,status,created_at)VALUES(?,?,0,?)");q.addBindValue(u);q.addBindValue(c);q.addBindValue(now());ok=q.exec();}if(ok)return DatabaseManager::instance().commitTransaction();DatabaseManager::instance().rollbackTransaction();if(e)*e="该电桩刚被占用，请重新选择";return false;}
bool PlatformService::start(int u,QString*e){QSqlQuery q(DatabaseManager::instance().db());q.prepare("UPDATE charging_order SET status=1,start_time=? WHERE user_id=? AND status=0");q.addBindValue(now());q.addBindValue(u);if(!q.exec()||q.numRowsAffected()!=1){if(e)*e="没有可开始的预约";return false;}return true;} bool PlatformService::settle(int u,QString*e){QSqlQuery q(DatabaseManager::instance().db());q.prepare("SELECT o.id,o.charger_id,station.price,charger.power,o.start_time FROM charging_order o join charger on charger.id=o.charger_id join station on station.id=charger.station_id WHERE o.user_id=? AND o.status=1");q.addBindValue(u);if(!q.exec()||!q.next()){if(e)*e="没有充电中的订单";return false;}int oid=q.value(0).toInt(),cid=q.value(1).toInt();int realSec=QDateTime::fromString(q.value(4).toString(),"yyyy-MM-dd HH:mm:ss").secsTo(QDateTime::currentDateTime()),simSec=realSec*60;double en=q.value(3).toDouble()*simSec/3600.0,am=en*q.value(2).toDouble();DatabaseManager::instance().beginTransaction();q.prepare("UPDATE charging_order SET status=2,end_time=?,energy=?,amount=? WHERE id=?");q.addBindValue(now());q.addBindValue(en);q.addBindValue(am);q.addBindValue(oid);bool ok=q.exec();q.prepare("UPDATE charger SET status=0,total_count=total_count+1,total_minutes=total_minutes+? WHERE id=?");q.addBindValue(simSec/60.);q.addBindValue(cid);ok&=q.exec();q.prepare("UPDATE user SET balance=max(0,balance-?) WHERE id=?");q.addBindValue(am);q.addBindValue(u);ok&=q.exec();return ok?DatabaseManager::instance().commitTransaction():(DatabaseManager::instance().rollbackTransaction(),false);} bool PlatformService::setUserStatus(int i,int s){QSqlQuery q(DatabaseManager::instance().db());q.prepare("UPDATE user SET status=? WHERE id=?");q.addBindValue(s);q.addBindValue(i);return q.exec();}bool PlatformService::setChargerStatus(int i,int s){QSqlQuery q(DatabaseManager::instance().db());q.prepare("UPDATE charger SET status=? WHERE id=?");q.addBindValue(s);q.addBindValue(i);return q.exec();}
bool PlatformService::cancelReservation(int u,QString*e){QSqlQuery q(DatabaseManager::instance().db());q.prepare("SELECT charger_id FROM charging_order WHERE user_id=? AND status=0 ORDER BY id DESC LIMIT 1");q.addBindValue(u);if(!q.exec()||!q.next()){if(e)*e="没有可取消的预约";return false;}const int cid=q.value(0).toInt();DatabaseManager::instance().beginTransaction();q.prepare("UPDATE charging_order SET status=3,end_time=? WHERE user_id=? AND status=0");q.addBindValue(now());q.addBindValue(u);bool ok=q.exec();q.prepare("UPDATE charger SET status=0 WHERE id=?");q.addBindValue(cid);ok&=q.exec();return ok?DatabaseManager::instance().commitTransaction():(DatabaseManager::instance().rollbackTransaction(),false);}
QVariantMap PlatformService::station(int id){QSqlQuery q(DatabaseManager::instance().db());q.prepare("SELECT s.*,count(c.id) total,sum(case when c.status=0 then 1 else 0 end) idle FROM station s LEFT JOIN charger c ON c.station_id=s.id WHERE s.id=? GROUP BY s.id");q.addBindValue(id);return q.exec()&&q.next()?map(q):QVariantMap{};}
QVariantMap PlatformService::chargerOverview(){QVariantMap r;QSqlQuery q(DatabaseManager::instance().db());q.exec("SELECT count(*) total,sum(case when status=0 then 1 else 0 end) idle,sum(case when status=1 then 1 else 0 end) busy,sum(case when status=2 then 1 else 0 end) fault FROM charger");if(q.next()){r["total"]=q.value("total");r["idle"]=q.value("idle");r["busy"]=q.value("busy");r["fault"]=q.value("fault");r["health"]=q.value("total").toInt()?100.0*(q.value("total").toInt()-q.value("fault").toInt())/q.value("total").toInt():0.0;}return r;}
QVariantList PlatformService::predictions(){QVariantList r;QSqlQuery q(DatabaseManager::instance().db());q.exec("SELECT p.*,s.name station_name FROM load_prediction p JOIN station s ON s.id=p.station_id ORDER BY p.target_time,p.station_id");while(q.next())r<<map(q);return r;}
bool PlatformService::restartCharger(int id,QString*e){QSqlQuery q(DatabaseManager::instance().db());q.prepare("SELECT status FROM charger WHERE id=?");q.addBindValue(id);if(!q.exec()||!q.next()){if(e)*e="电桩不存在";return false;}if(q.value(0).toInt()==1){if(e)*e="该电桩正在充电，不能远程重启";return false;}DatabaseManager::instance().beginTransaction();q.prepare("UPDATE charger SET status=0 WHERE id=?");q.addBindValue(id);bool ok=q.exec();q.prepare("INSERT INTO ops_log(charger_id,action,created_at) VALUES(?,?,?)");q.addBindValue(id);q.addBindValue("远程重启并恢复正常");q.addBindValue(now());ok&=q.exec();return ok?DatabaseManager::instance().commitTransaction():(DatabaseManager::instance().rollbackTransaction(),false);}
bool PlatformService::saveStation(int id,const QString&name,const QString&addr,double lon,double lat,double price,QString*e){if(name.trimmed().isEmpty()||lon < -180||lon>180||lat<-90||lat>90||price<=0){if(e)*e="请检查站名、经纬度和单价";return false;}QSqlQuery q(DatabaseManager::instance().db());q.prepare(id?"UPDATE station SET name=?,address=?,longitude=?,latitude=?,price=? WHERE id=?":"INSERT INTO station(name,address,longitude,latitude,price) VALUES(?,?,?,?,?)");q.addBindValue(name.trimmed());q.addBindValue(addr.trimmed());q.addBindValue(lon);q.addBindValue(lat);q.addBindValue(price);if(id)q.addBindValue(id);return q.exec();}
bool PlatformService::deleteStation(int id,QString*e){QSqlQuery q(DatabaseManager::instance().db());q.prepare("SELECT count(*) FROM charger WHERE station_id=?");q.addBindValue(id);q.exec();q.next();if(q.value(0).toInt()){if(e)*e="该电站下仍有电桩，禁止删除";return false;}q.prepare("DELETE FROM station WHERE id=?");q.addBindValue(id);return q.exec();}
bool PlatformService::saveCharger(int id,int stationId,const QString&code,const QString&type,double power,QString*e){if(stationId<=0||code.trimmed().isEmpty()||power<=0||(type!="快充"&&type!="慢充")){if(e)*e="请填写有效的电桩信息";return false;}QSqlQuery q(DatabaseManager::instance().db());q.prepare(id?"UPDATE charger SET station_id=?,code=?,type=?,power=? WHERE id=?":"INSERT INTO charger(station_id,code,type,power,status) VALUES(?,?,?,?,0)");q.addBindValue(stationId);q.addBindValue(code.trimmed());q.addBindValue(type);q.addBindValue(power);if(id)q.addBindValue(id);if(!q.exec()){if(e)*e="编号已存在或保存失败";return false;}return true;}
bool PlatformService::deleteCharger(int id,QString*e){QSqlQuery q(DatabaseManager::instance().db());q.prepare("DELETE FROM charger WHERE id=? AND status<>1");q.addBindValue(id);if(!q.exec()||q.numRowsAffected()!=1){if(e)*e="使用中的电桩禁止删除";return false;}return true;}
