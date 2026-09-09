#include "MockPlatform.h"
#include "DeviceConnection.h"

#include <QDateTime>
#include <QHostAddress>
#include <QWebSocket>
#include <QWebSocketServer>

using device_link::DeviceConnection;

MockPlatform::MockPlatform(QObject *parent)
    : QObject(parent)
    , m_server(new QWebSocketServer(QStringLiteral("PileSelfTestMockPlatform"),
                                    QWebSocketServer::NonSecureMode, this))
{
    connect(m_server, &QWebSocketServer::newConnection, this, &MockPlatform::onNewConnection);
}

MockPlatform::~MockPlatform() = default;

bool MockPlatform::listen(quint16 port)
{
    if (!m_server->listen(QHostAddress::LocalHost, port)) {
        emit logMessage(QStringLiteral("模拟平台监听失败:%1").arg(m_server->errorString()));
        return false;
    }
    emit logMessage(QStringLiteral("模拟平台已监听 ws://127.0.0.1:%1")
                        .arg(m_server->serverPort()));
    return true;
}

quint16 MockPlatform::port() const
{
    return m_server->serverPort();
}

void MockPlatform::onNewConnection()
{
    while (m_server->hasPendingConnections()) {
        QWebSocket *sock = m_server->nextPendingConnection();
        if (m_pile) {
            DeviceConnection *old = m_pile;
            m_pile = nullptr;
            old->disconnectFromServer(false);
            old->deleteLater();
        }
        auto *conn = new DeviceConnection(sock, this);
        m_pile = conn;
        connect(conn, &DeviceConnection::requestReceived, this,
                [this, conn](const QString &action, const QJsonObject &payload,
                             const QString &messageId) {
                    onRequest(conn, action, payload, messageId);
                });
        connect(conn, &DeviceConnection::disconnected, this, [this, conn]() {
            if (m_pile == conn)
                m_pile = nullptr;
            conn->deleteLater();
            emit logMessage(QStringLiteral("模拟平台:桩连接已断开"));
        });
        emit logMessage(QStringLiteral("模拟平台:收到桩连接,等待开机注册"));
    }
}

void MockPlatform::onRequest(DeviceConnection *conn, const QString &action,
                             const QJsonObject &payload, const QString &messageId)
{
    if (action == QLatin1String("BootNotification")) {
        conn->sendResponse(messageId,
                           QJsonObject{{QStringLiteral("status"), QStringLiteral("Accepted")},
                                       {QStringLiteral("currentTime"),
                                        QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},
                                       {QStringLiteral("interval"), 10}});
        emit logMessage(QStringLiteral("模拟平台:已接受桩 %1 开机注册")
                            .arg(payload.value(QStringLiteral("chargePointSerialNumber"))
                                     .toString()));
        return;
    }
    if (action == QLatin1String("Heartbeat")) {
        conn->sendResponse(messageId,
                           QJsonObject{{QStringLiteral("currentTime"),
                                        QDateTime::currentDateTimeUtc().toString(Qt::ISODate)}});
        return;
    }
    if (action == QLatin1String("StatusNotification")) {
        conn->sendResponse(messageId, QJsonObject{});
        emit logMessage(QStringLiteral("模拟平台:收到状态上报(%1)")
                            .arg(payload.value(QStringLiteral("status")).toString()));
        return;
    }
    if (action == QLatin1String("MeterValues")) {
        // 计量上报静默应答,界面数字本身即演示效果
        conn->sendResponse(messageId, QJsonObject{});
        return;
    }
    conn->sendError(messageId, QStringLiteral("NotSupported"),
                    QStringLiteral("unknown action"));
}

void MockPlatform::sendToPile(const QString &action, const QJsonObject &payload)
{
    if (!m_pile || !m_pile->isConnected()) {
        emit logMessage(QStringLiteral("模拟平台:桩尚未连接,无法下发指令"));
        return;
    }
    m_pile->sendRequest(action, payload, [](bool, const QJsonObject &) {});
    emit logMessage(QStringLiteral("模拟平台:已下发指令 %1").arg(action));
}
