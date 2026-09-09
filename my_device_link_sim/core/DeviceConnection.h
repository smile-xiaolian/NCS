#pragma once

#include "PendingRequestTable.h"

#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QUrl>

#include <functional>

class QWebSocket;

namespace device_link {

// 通用连接封装,客户端与服务端两端都能用,完全不知道业务 Action 含义。
// 客户端模式:持有 socket,负责连接与指数退避重连。
// 服务端模式:包装 QWebSocketServer 接收到的已建立连接。
class DeviceConnection : public QObject {
    Q_OBJECT
public:
    using SimpleCallback = std::function<void(bool ok, const QJsonObject &result)>;

    explicit DeviceConnection(const QUrl &serverUrl, QObject *parent = nullptr);
    explicit DeviceConnection(QWebSocket *acceptedSocket, QObject *parent = nullptr);
    ~DeviceConnection() override;

    QString deviceId() const { return m_deviceId; }
    void setDeviceId(const QString &id) { m_deviceId = id; }

    bool isConnected() const;
    QUrl serverUrl() const { return m_serverUrl; }

    void connectToServer();
    // allowReconnect=true: 关闭后自动重连(故障注入用)
    void disconnectFromServer(bool allowReconnect = false);
    void setAutoReconnect(bool enabled) { m_autoReconnect = enabled; }

    // 平台侧连接不活动看门狗:ms 内无任何消息则判定失活,0 表示关闭。
    void setInactivityTimeoutMs(int ms);
    void noteActivity();

    // 发请求,响应或超时都会回调;返回本次消息 ID。
    QString sendRequest(const QString &action, const QJsonObject &payload,
                        SimpleCallback callback, int timeoutMs = 5000);
    void sendResponse(const QString &messageId, const QJsonObject &result);
    void sendError(const QString &messageId, const QString &errorCode, const QString &errorDesc);

signals:
    void requestReceived(const QString &action, const QJsonObject &payload,
                         const QString &messageId);
    void connected();
    void disconnected();
    void heartbeatTimeout();           // 平台侧:超过阈值未收到任何消息
    void orphanResponse(const QString &messageId);

private slots:
    void onSocketConnected();
    void onSocketDisconnected();
    void onTextMessage(const QString &message);
    void attemptConnect();
    void onInactivityTick();

private:
    void bindSocket(QWebSocket *socket);
    void scheduleReconnect();
    QString nextMessageId();

    QWebSocket *m_socket = nullptr;
    PendingRequestTable m_pending;
    QUrl m_serverUrl;
    QString m_deviceId;
    bool m_clientMode = false;
    bool m_autoReconnect = true;
    bool m_manualClose = false;
    int m_reconnectAttempt = 0;
    int m_msgSeq = 0;
    int m_inactivityTimeoutMs = 0;
    QTimer m_inactivityTimer;
};

} // namespace device_link
