#pragma once

#include <QJsonObject>
#include <QObject>
#include <QString>

class QWebSocketServer;

namespace device_link {
class DeviceConnection;
}

// 内置模拟平台(桩端独立自测用):在本进程监听本地端口,自动应答开机注册/心跳/
// 状态/计量等请求,并可模拟平台向下发 RemoteStart/RemoteStop/RemoteReset。
// 无需启动真实 platform_app 即可让桩跑通完整通信流程。
class MockPlatform : public QObject {
    Q_OBJECT
public:
    explicit MockPlatform(QObject *parent = nullptr);
    ~MockPlatform() override;

    bool listen(quint16 port);
    quint16 port() const;
    bool hasPile() const { return m_pile != nullptr; }

    // 模拟平台主动下发指令到已连接的桩
    void sendToPile(const QString &action, const QJsonObject &payload);

signals:
    void logMessage(const QString &text);

private:
    void onNewConnection();
    void onRequest(device_link::DeviceConnection *conn, const QString &action,
                   const QJsonObject &payload, const QString &messageId);

    QWebSocketServer *m_server = nullptr;
    device_link::DeviceConnection *m_pile = nullptr;
};
