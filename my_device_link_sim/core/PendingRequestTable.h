#pragma once

#include <QHash>
#include <QJsonObject>
#include <QObject>
#include <QString>

#include <functional>

class QTimer;

namespace device_link {

using RequestCallback = std::function<void(bool ok, const QJsonObject &result,
                                           const QString &errorCode, const QString &errorDesc)>;

// messageId -> 回调 + 超时定时器:请求-响应关联与超时管理。
// 响应找不到对应未完成请求时发 orphanResponse(上层记警告,不崩溃)。
class PendingRequestTable : public QObject {
    Q_OBJECT
public:
    explicit PendingRequestTable(QObject *parent = nullptr);

    void add(const QString &messageId, RequestCallback callback, int timeoutMs);
    bool completeSuccess(const QString &messageId, const QJsonObject &result);
    bool completeError(const QString &messageId, const QString &errorCode, const QString &errorDesc);
    void clearAll();

signals:
    void orphanResponse(const QString &messageId);

private:
    struct Entry {
        RequestCallback callback;
        QTimer *timer = nullptr;
    };

    void finish(const QString &messageId, bool ok, const QJsonObject &result,
                const QString &errorCode, const QString &errorDesc);

    QHash<QString, Entry> m_entries;
};

} // namespace device_link
