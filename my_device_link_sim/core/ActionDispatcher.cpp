#include "ActionDispatcher.h"

#include <utility>

namespace device_link {

ActionDispatcher::ActionDispatcher(QObject *parent)
    : QObject(parent)
{
}

void ActionDispatcher::registerHandler(const QString &action, Handler handler)
{
    m_handlers[action.toStdString()] = std::move(handler);
}

bool ActionDispatcher::dispatch(const QString &action, const QJsonObject &payload,
                                const QString &messageId)
{
    const auto it = m_handlers.find(action.toStdString());
    if (it == m_handlers.end()) {
        emit unhandledAction(action, payload, messageId);
        return false;
    }
    it->second(payload, messageId);
    return true;
}

} // namespace device_link
