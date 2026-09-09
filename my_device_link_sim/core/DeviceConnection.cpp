#include "DeviceConnection.h"
#include "MessageFrame.h"

#include <QDateTime>
#include <QDebug>
#include <QUuid>
#include <QWebSocket>

namespace device_link {

DeviceConnection::DeviceConnection(const QUrl &serverUrl, QObject *parent)
    : QObject(parent)
    , m_pending(this)
    , m_serverUrl(serverUrl)
    , m_clientMode(true)
{
    connect(&m_pending, &PendingRequestTable::orphanResponse, this,
            &DeviceConnection::orphanResponse);
    m_inactivityTimer.setSingleShot(false);
    connect(&m_inactivityTimer, &QTimer::timeout, this, &DeviceConnection::onInactivityTick);
    bindSocket(new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this));
}

DeviceConnection::DeviceConnection(QWebSocket *acceptedSocket, QObject *parent)
    : QObject(parent)
    , m_pending(this)
    , m_clientMode(false)
    , m_autoReconnect(false)
{
    connect(&m_pending, &PendingRequestTable::orphanResponse, this,
            &DeviceConnection::orphanResponse);
    m_inactivityTimer.setSingleShot(false);
    connect(&m_inactivityTimer, &QTimer::timeout, this, &DeviceConnection::onInactivityTick);
    bindSocket(acceptedSocket);
    // 已建立的连接补发 connected 信号(等事件循环开始后再发)
    if (m_socket && m_socket->state() == QAbstractSocket::ConnectedState)
        QMetaObject::invokeMethod(this, &DeviceConnection::onSocketConnected, Qt::QueuedConnection);
}

DeviceConnection::~DeviceConnection()
{
    m_manualClose = true;
    m_autoReconnect = false;
    if (m_socket)
        m_socket->abort();
}

void DeviceConnection::bindSocket(QWebSocket *socket)
{
    if (m_socket && m_socket != socket) {
        m_socket->disconnect(this);
        if (m_socket->parent() == this)
            m_socket->deleteLater();
    }
    m_socket = socket;
    if (!m_socket)
        return;
    if (!m_socket->parent())
        m_socket->setParent(this);

    connect(m_socket, &QWebSocket::connected, this, &DeviceConnection::onSocketConnected);
    connect(m_socket, &QWebSocket::disconnected, this, &DeviceConnection::onSocketDisconnected);
    connect(m_socket, &QWebSocket::textMessageReceived, this, &DeviceConnection::onTextMessage);
}

bool DeviceConnection::isConnected() const
{
    return m_socket && m_socket->state() == QAbstractSocket::ConnectedState;
}

void DeviceConnection::connectToServer()
{
    if (!m_clientMode || !m_socket)
        return;
    m_manualClose = false;
    if (m_socket->state() == QAbstractSocket::ConnectedState
        || m_socket->state() == QAbstractSocket::ConnectingState)
        return;
    attemptConnect();
}

void DeviceConnection::disconnectFromServer(bool allowReconnect)
{
    m_manualClose = !allowReconnect;
    m_autoReconnect = allowReconnect;
    if (m_socket)
        m_socket->close();
}

void DeviceConnection::setInactivityTimeoutMs(int ms)
{
    m_inactivityTimeoutMs = ms;
    m_inactivityTimer.stop();
    if (ms > 0 && isConnected()) {
        m_inactivityTimer.start(qMax(1000, ms / 3));
        noteActivity();
    }
}

void DeviceConnection::noteActivity()
{
    if (!m_socket)
        return;
    m_socket->setProperty("lastActivityMs", QDateTime::currentMSecsSinceEpoch());
}

void DeviceConnection::attemptConnect()
{
    if (!m_clientMode || !m_socket || m_manualClose)
        return;
    qInfo() << QStringLiteral("设备连接:正在连接 %1").arg(m_serverUrl.toString());
    m_socket->open(m_serverUrl);
}

void DeviceConnection::scheduleReconnect()
{
    if (!m_clientMode || !m_autoReconnect || m_manualClose)
        return;
    // 1s, 2s, 4s, ... 封顶 30s;位移上限 5 防止溢出
    const int delayMs = qMin(1000 * (1 << qMin(m_reconnectAttempt, 5)), 30000);
    ++m_reconnectAttempt;
    qInfo() << QStringLiteral("设备连接:%1 ms 后重连(第 %2 次)").arg(delayMs).arg(m_reconnectAttempt);
    QTimer::singleShot(delayMs, this, &DeviceConnection::attemptConnect);
}

QString DeviceConnection::nextMessageId()
{
    return QStringLiteral("%1-%2")
        .arg(QUuid::createUuid().toString(QUuid::WithoutBraces).left(8))
        .arg(++m_msgSeq);
}

QString DeviceConnection::sendRequest(const QString &action, const QJsonObject &payload,
                                      SimpleCallback callback, int timeoutMs)
{
    const QString messageId = nextMessageId();
    if (!isConnected()) {
        if (callback)
            callback(false, {});
        return messageId;
    }

    MessageFrame frame;
    frame.type = MessageFrame::Call;
    frame.messageId = messageId;
    frame.action = action;
    frame.payload = payload;

    m_pending.add(
        messageId,
        [callback](bool ok, const QJsonObject &result, const QString &, const QString &) {
            if (callback)
                callback(ok, result);
        },
        timeoutMs);

    m_socket->sendTextMessage(QString::fromUtf8(frame.toJson()));
    return messageId;
}

void DeviceConnection::sendResponse(const QString &messageId, const QJsonObject &result)
{
    if (!isConnected())
        return;
    MessageFrame frame;
    frame.type = MessageFrame::CallResult;
    frame.messageId = messageId;
    frame.payload = result;
    m_socket->sendTextMessage(QString::fromUtf8(frame.toJson()));
}

void DeviceConnection::sendError(const QString &messageId, const QString &errorCode,
                                 const QString &errorDesc)
{
    if (!isConnected())
        return;
    MessageFrame frame;
    frame.type = MessageFrame::CallError;
    frame.messageId = messageId;
    frame.errorCode = errorCode;
    frame.errorDesc = errorDesc;
    frame.payload = QJsonObject{};
    m_socket->sendTextMessage(QString::fromUtf8(frame.toJson()));
}

void DeviceConnection::onSocketConnected()
{
    m_reconnectAttempt = 0;
    noteActivity();
    if (m_inactivityTimeoutMs > 0)
        m_inactivityTimer.start(qMax(1000, m_inactivityTimeoutMs / 3));
    emit connected();
}

void DeviceConnection::onSocketDisconnected()
{
    m_inactivityTimer.stop();
    m_pending.clearAll();
    emit disconnected();
    if (m_clientMode && m_autoReconnect && !m_manualClose)
        scheduleReconnect();
}

void DeviceConnection::onTextMessage(const QString &message)
{
    noteActivity();
    const auto frameOpt = MessageFrame::fromJson(message.toUtf8());
    if (!frameOpt) {
        qWarning() << QStringLiteral("设备连接:无法解析的消息帧:%1").arg(message);
        return;
    }

    const MessageFrame &frame = *frameOpt;
    switch (frame.type) {
    case MessageFrame::Call:
        emit requestReceived(frame.action, frame.payload, frame.messageId);
        break;
    case MessageFrame::CallResult:
        m_pending.completeSuccess(frame.messageId, frame.payload);
        break;
    case MessageFrame::CallError:
        m_pending.completeError(frame.messageId, frame.errorCode, frame.errorDesc);
        break;
    }
}

void DeviceConnection::onInactivityTick()
{
    if (m_inactivityTimeoutMs <= 0 || !m_socket)
        return;
    const qint64 last = m_socket->property("lastActivityMs").toLongLong();
    if (last <= 0)
        return;
    if (QDateTime::currentMSecsSinceEpoch() - last > m_inactivityTimeoutMs) {
        qWarning() << QStringLiteral("设备连接:设备 %1 超过阈值无消息,判定失活")
                          .arg(m_deviceId);
        emit heartbeatTimeout();
        m_socket->abort();
    }
}

} // namespace device_link
