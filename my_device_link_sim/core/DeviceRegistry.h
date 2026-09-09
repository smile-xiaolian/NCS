#pragma once

#include <QHash>
#include <QObject>
#include <QString>

namespace device_link {

class DeviceConnection;

// 平台侧:多个 DeviceConnection 的管理容器。
class DeviceRegistry : public QObject {
    Q_OBJECT
public:
    explicit DeviceRegistry(QObject *parent = nullptr);

    void add(const QString &deviceId, DeviceConnection *connection);
    void remove(const QString &deviceId);
    void removeByConnection(DeviceConnection *connection);
    DeviceConnection *get(const QString &deviceId) const;
    QStringList deviceIds() const;
    int count() const { return m_devices.size(); }

signals:
    void deviceAdded(const QString &deviceId);
    void deviceRemoved(const QString &deviceId);

private:
    QHash<QString, DeviceConnection *> m_devices;
};

} // namespace device_link
