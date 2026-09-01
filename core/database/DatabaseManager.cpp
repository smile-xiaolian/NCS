#include "DatabaseManager.h"
#include <QSqlError>
#include <QDebug>

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