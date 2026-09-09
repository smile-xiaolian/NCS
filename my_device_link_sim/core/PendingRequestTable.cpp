#include "PendingRequestTable.h"

#include <QTimer>

#include <utility>

namespace device_link {

PendingRequestTable::PendingRequestTable(QObject *parent)
    : QObject(parent)
{
}

void PendingRequestTable::add(const QString &messageId, RequestCallback callback, int timeoutMs)
{
    Entry entry;
    entry.callback = std::move(callback);
    if (timeoutMs > 0) {
        auto *timer = new QTimer(this);
        timer->setSingleShot(true);
        connect(timer, &QTimer::timeout, this, [this, messageId]() {
            finish(messageId, false, QJsonObject{}, QStringLiteral("Timeout"),
                   QStringLiteral("request timed out"));
        });
        timer->start(timeoutMs);
        entry.timer = timer;
    }
    m_entries.insert(messageId, entry);
}

void PendingRequestTable::finish(const QString &messageId, bool ok, const QJsonObject &result,
                                 const QString &errorCode, const QString &errorDesc)
{
    const auto it = m_entries.find(messageId);
    if (it == m_entries.end())
        return;

    Entry entry = it.value();
    m_entries.erase(it);

    if (entry.timer) {
        entry.timer->stop();
        entry.timer->deleteLater();
    }
    if (entry.callback)
        entry.callback(ok, result, errorCode, errorDesc);
}

bool PendingRequestTable::completeSuccess(const QString &messageId, const QJsonObject &result)
{
    if (!m_entries.contains(messageId)) {
        emit orphanResponse(messageId);
        return false;
    }
    finish(messageId, true, result, QString{}, QString{});
    return true;
}

bool PendingRequestTable::completeError(const QString &messageId, const QString &errorCode,
                                        const QString &errorDesc)
{
    if (!m_entries.contains(messageId)) {
        emit orphanResponse(messageId);
        return false;
    }
    finish(messageId, false, QJsonObject{}, errorCode, errorDesc);
    return true;
}

void PendingRequestTable::clearAll()
{
    for (const auto &entry : std::as_const(m_entries)) {
        if (entry.timer) {
            entry.timer->stop();
            entry.timer->deleteLater();
        }
    }
    m_entries.clear();
}

} // namespace device_link
