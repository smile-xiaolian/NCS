#include "DatabaseManager.h"
#include <QSqlError>
#include <QDebug>
#include <QSqlQuery>

DatabaseManager& DatabaseManager::instance()
{
    static DatabaseManager instance;
    return instance;
}

bool DatabaseManager::open(const QString& path)
{
    if (m_isOpen) {
        close();
    }

    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(path);

    if (!m_db.open()) {
        qDebug() << "Database open failed:" << m_db.lastError().text();
        return false;
    }

    // 开启 WAL 模式 (UC-D-03)
    QSqlQuery query(m_db);
    query.exec("PRAGMA journal_mode = WAL");
    query.exec("PRAGMA foreign_keys = ON");
    // 多端(ncs_user/ncs_admin/模拟器平台)共享同一库:写锁相撞时等待 5 秒再重试,
    // 而不是立刻报 "database is locked" 导致初始化失败
    query.exec("PRAGMA busy_timeout = 5000");

    m_isOpen = true;
    return true;
}

void DatabaseManager::close()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
    m_isOpen = false;
}

bool DatabaseManager::isOpen() const
{
    return m_isOpen;
}

bool DatabaseManager::beginTransaction()
{
    return m_db.transaction();
}

bool DatabaseManager::commitTransaction()
{
    return m_db.commit();
}

bool DatabaseManager::rollbackTransaction()
{
    return m_db.rollback();
}
