#include "DeviceRegistry.h"

namespace device_link {

DeviceRegistry::DeviceRegistry(QObject *parent)
    : QObject(parent)
{
}

void DeviceRegistry::add(const QString &deviceId, DeviceConnection *connection)
{
    if (deviceId.isEmpty() || !connection)
        return;
    m_devices.insert(deviceId, connection);
    emit deviceAdded(deviceId);
}

void DeviceRegistry::remove(const QString &deviceId)
{
    if (m_devices.remove(deviceId) > 0)
        emit deviceRemoved(deviceId);
}

void DeviceRegistry::removeByConnection(DeviceConnection *connection)
{
    for (auto it = m_devices.begin(); it != m_devices.end(); ++it) {
        if (it.value() == connection) {
            const QString id = it.key();
            m_devices.erase(it);
            emit deviceRemoved(id);
            return;
        }
    }
}

DeviceConnection *DeviceRegistry::get(const QString &deviceId) const
{
    return m_devices.value(deviceId, nullptr);
}

QStringList DeviceRegistry::deviceIds() const
{
    return m_devices.keys();
}

} // namespace device_link
