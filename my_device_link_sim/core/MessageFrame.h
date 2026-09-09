#pragma once

#include <QByteArray>
#include <QJsonObject>
#include <QString>

#include <optional>

namespace device_link {

// 类 OCPP 数组格式消息,只做编解码,不关心业务含义:
//   请求:    [2, messageId, action, payload]
//   成功响应: [3, messageId, payload]
//   失败响应: [4, messageId, errorCode, errorDesc, payload?]
struct MessageFrame {
    enum Type { Call = 2, CallResult = 3, CallError = 4 };

    Type type = Call;
    QString messageId;
    QString action;    // 仅 Call 有效
    QJsonObject payload;
    QString errorCode; // 仅 CallError 有效
    QString errorDesc; // 仅 CallError 有效

    QByteArray toJson() const;
    static std::optional<MessageFrame> fromJson(const QByteArray &data);
};

} // namespace device_link
