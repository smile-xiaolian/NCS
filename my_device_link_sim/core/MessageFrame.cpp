#include "MessageFrame.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>

namespace device_link {

QByteArray MessageFrame::toJson() const
{
    QJsonArray arr;
    arr.append(static_cast<int>(type));
    arr.append(messageId);

    switch (type) {
    case Call:
        arr.append(action);
        arr.append(payload);
        break;
    case CallResult:
        arr.append(payload);
        break;
    case CallError:
        arr.append(errorCode);
        arr.append(errorDesc);
        arr.append(payload.isEmpty() ? QJsonObject{} : payload);
        break;
    }

    return QJsonDocument(arr).toJson(QJsonDocument::Compact);
}

std::optional<MessageFrame> MessageFrame::fromJson(const QByteArray &data)
{
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isArray())
        return std::nullopt;

    const QJsonArray arr = doc.array();
    if (arr.size() < 3 || !arr.at(0).isDouble() || !arr.at(1).isString())
        return std::nullopt;

    const int typeVal = arr.at(0).toInt();
    if (typeVal != Call && typeVal != CallResult && typeVal != CallError)
        return std::nullopt;

    MessageFrame frame;
    frame.type = static_cast<Type>(typeVal);
    frame.messageId = arr.at(1).toString();
    if (frame.messageId.isEmpty())
        return std::nullopt;

    switch (frame.type) {
    case Call:
        if (arr.size() < 4 || !arr.at(2).isString() || !arr.at(3).isObject())
            return std::nullopt;
        frame.action = arr.at(2).toString();
        if (frame.action.isEmpty())
            return std::nullopt;
        frame.payload = arr.at(3).toObject();
        break;
    case CallResult:
        if (!arr.at(2).isObject())
            return std::nullopt;
        frame.payload = arr.at(2).toObject();
        break;
    case CallError:
        if (arr.size() < 4 || !arr.at(2).isString() || !arr.at(3).isString())
            return std::nullopt;
        frame.errorCode = arr.at(2).toString();
        frame.errorDesc = arr.at(3).toString();
        if (arr.size() >= 5 && arr.at(4).isObject())
            frame.payload = arr.at(4).toObject();
        break;
    }

    return frame;
}

} // namespace device_link
