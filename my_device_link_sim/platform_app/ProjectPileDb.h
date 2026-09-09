#pragma once

#include <QList>
#include <QString>

class QSqlDatabase;

// NCS 主工程 SQLite 数据库(charge_platform.db)桥接。
// 只读取/回写主工程已建好的表(charger / station / ops_log),不创建也不修改表结构;
// 数据库不存在或不可用时,平台端自动退回独立演示模式,不依赖主工程。
class ProjectPileDb {
public:
    struct Charger {
        QString code;         // 电桩编号,如 S1-01(charger.code,主工程中唯一)
        QString type;         // 快充 / 慢充
        double powerKw = 0.0; // 额定功率
        QString stationName;  // 所属电站名(charger.station_id -> station.name)
    };

    explicit ProjectPileDb(const QString &dbPath);
    ~ProjectPileDb();

    // 与主工程一致的默认数据库位置:
    // Linux ~/.local/share/NCS/charge_platform.db,Windows %LOCALAPPDATA%/NCS/charge_platform.db
    static QString defaultDatabasePath();

    bool open(QString *error = nullptr);
    bool isOpen() const { return m_open; }
    QString databasePath() const { return m_path; }

    // 项目里登记的全部充电桩(仅当表存在且可读时返回)
    QList<Charger> chargers() const;
    bool findByCode(const QString &code, Charger *out) const;

    // 回写电桩状态(0 空闲 / 1 充电中 / 2 故障)并追加一条运营日志,
    // 仅在编号命中项目电桩时生效,返回是否命中。
    bool updateState(const QString &code, int status, const QString &action);

private:
    void close();

    QString m_path;
    bool m_open = false;
};