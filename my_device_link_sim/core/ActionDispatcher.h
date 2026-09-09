#pragma once

#include <QJsonObject>
#include <QObject>
#include <QString>

#include <functional>
#include <unordered_map>

namespace device_link {

// 按 Action 名称把入站 Call 帧路由到已注册的处理函数。
class ActionDispatcher : public QObject {
    Q_OBJECT
public:
    using Handler = std::function<void(const QJsonObject &payload, const QString &messageId)>;

    explicit ActionDispatcher(QObject *parent = nullptr);

    void registerHandler(const QString &action, Handler handler);
    // 命中返回 true;未注册的 Action 发 unhandledAction 并返回 false。
    bool dispatch(const QString &action, const QJsonObject &payload, const QString &messageId);

signals:
    void unhandledAction(const QString &action, const QJsonObject &payload,
                         const QString &messageId);

private:
    std::unordered_map<std::string, Handler> m_handlers;
};

} // namespace device_link
