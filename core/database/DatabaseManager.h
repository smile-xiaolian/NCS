#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QSqlDatabase>
#include <QString>

class DatabaseManager
{
public:
    static DatabaseManager& instance();

    bool open(const QString& path);
    void close();
    bool isOpen() const;

    QSqlDatabase& db() { return m_db; }

    // 事务操作
    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();

private:
    DatabaseManager() = default;
    ~DatabaseManager() = default;

    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    QSqlDatabase m_db;
    bool m_isOpen = false;
};

#endif // DATABASEMANAGER_H