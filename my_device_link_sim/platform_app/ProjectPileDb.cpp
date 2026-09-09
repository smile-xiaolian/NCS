#include "ProjectPileDb.h"

#include <QDateTime>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>

namespace {

const char kConnName[] = "ncs_project_piles";

ProjectPileDb::Charger chargerFromRow(const QSqlQuery &q)
{
    ProjectPileDb::Charger c;
    c.code = q.value(0).toString();
    c.type = q.value(1).toString();
    c.powerKw = q.value(2).toDouble();
    c.stationName = q.value(3).toString();
    return c;
}

} // namespace

ProjectPileDb::ProjectPileDb(const QString &dbPath)
    : m_path(dbPath)
{
}

ProjectPileDb::~ProjectPileDb()
{
    close();
}

QString ProjectPileDb::defaultDatabasePath()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
           + QStringLiteral("/NCS/charge_platform.db");
}

bool ProjectPileDb::open(QString *error)
{
    if (m_open)
        return true;
    if (m_path.isEmpty() || !QFileInfo::exists(m_path)) {
        if (error)
            *error = QStringLiteral("未找到数据库文件:%1").arg(m_path);
        return false;
    }

    QSqlDatabase db;
    if (QSqlDatabase::contains(QLatin1String(kConnName))) {
        db = QSqlDatabase::database(QLatin1String(kConnName), false);
        if (db.isOpen()) {
            m_open = true;
            return true;
        }
    } else {
        db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QLatin1String(kConnName));
    }
    db.setDatabaseName(m_path);
    if (!db.open()) {
        if (error)
            *error = db.lastError().text();
        return false;
    }

    // 主工程首次运行时会建表;缺表说明数据库尚未初始化,按不可用处理
    QSqlQuery probe(db);
    probe.prepare("SELECT count(*) FROM charger");
    if (!probe.exec()) {
        if (error)
            *error = QStringLiteral("数据库缺少 charger 表:%1")
                         .arg(probe.lastError().text());
        close();
        return false;
    }
    m_open = true;
    return true;
}

void ProjectPileDb::close()
{
    if (QSqlDatabase::contains(QLatin1String(kConnName))) {
        {
            QSqlDatabase db = QSqlDatabase::database(QLatin1String(kConnName), false);
            if (db.isValid() && db.isOpen())
                db.close();
        }
        QSqlDatabase::removeDatabase(QLatin1String(kConnName));
    }
    m_open = false;
}

QList<ProjectPileDb::Charger> ProjectPileDb::chargers() const
{
    QList<Charger> out;
    if (!m_open)
        return out;

    QSqlQuery q(QSqlDatabase::database(QLatin1String(kConnName), false));
    q.prepare("SELECT c.code,c.type,c.power,COALESCE(s.name,'') "
              "FROM charger c LEFT JOIN station s ON s.id=c.station_id "
              "ORDER BY c.code");
    if (!q.exec())
        return out;
    while (q.next())
        out.append(chargerFromRow(q));
    return out;
}

bool ProjectPileDb::findByCode(const QString &code, Charger *out) const
{
    if (!m_open || code.isEmpty() || !out)
        return false;

    QSqlQuery q(QSqlDatabase::database(QLatin1String(kConnName), false));
    q.prepare("SELECT c.code,c.type,c.power,COALESCE(s.name,'') "
              "FROM charger c LEFT JOIN station s ON s.id=c.station_id "
              "WHERE c.code=? LIMIT 1");
    q.addBindValue(code);
    if (!q.exec() || !q.next())
        return false;
    *out = chargerFromRow(q);
    return true;
}

bool ProjectPileDb::updateState(const QString &code, int status, const QString &action)
{
    if (!m_open || code.isEmpty())
        return false;

    QSqlDatabase db = QSqlDatabase::database(QLatin1String(kConnName), false);
    if (!db.isOpen())
        return false;

    QSqlQuery q(db);
    q.prepare("SELECT id,status FROM charger WHERE code=?");
    q.addBindValue(code);
    if (!q.exec() || !q.next())
        return false; // 非项目在册电桩,不写入

    const int id = q.value(0).toInt();
    const int oldStatus = q.value(1).toInt();

    if (oldStatus != status) {
        q.prepare("UPDATE charger SET status=? WHERE id=?");
        q.addBindValue(status);
        q.addBindValue(id);
        if (!q.exec())
            return false;
    }

    if (!action.isEmpty()) {
        q.prepare("INSERT INTO ops_log(charger_id,action,created_at) VALUES(?,?,?)");
        q.addBindValue(id);
        q.addBindValue(action);
        q.addBindValue(QDateTime::currentDateTime().toString(
            QStringLiteral("yyyy-MM-dd HH:mm:ss")));
        if (!q.exec())
            return false;
    }
    return true;
}