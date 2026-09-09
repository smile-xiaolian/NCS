#include "PileController.h"

#include "DeviceConnection.h"

#include <QDateTime>
#include <QRandomGenerator>

using device_link::DeviceConnection;

PileController::PileController(const QString &pileCode, const QString &model, const QUrl &serverUrl,
                               QObject *parent)
    : QObject(parent)
    , m_pileCode(pileCode)
    , m_model(model)
    , m_conn(new DeviceConnection(serverUrl, this))
{
    m_conn->setDeviceId(pileCode);
    m_conn->setAutoReconnect(true);

    connect(m_conn, &DeviceConnection::connected, this, &PileController::onConnected);
    connect(m_conn, &DeviceConnection::disconnected, this, &PileController::onDisconnected);
    connect(m_conn, &DeviceConnection::requestReceived, this, &PileController::onRequest);
    connect(m_conn, &DeviceConnection::orphanResponse, this, [this](const QString &id) {
        emit logMessage(QStringLiteral("警告:收到无匹配请求的响应 %1").arg(id));
    });

    // 心跳保活:每 10 秒
    m_heartbeatTimer.setInterval(10000);
    connect(&m_heartbeatTimer, &QTimer::timeout, this, &PileController::sendHeartbeat);

    // 充电中计量上报:每 2 秒
    m_meterTimer.setInterval(2000);
    connect(&m_meterTimer, &QTimer::timeout, this, &PileController::sendMeter);
}

QString PileController::stateText() const
{
    switch (m_state) {
    case State::Booting:  return QStringLiteral("Booting");
    case State::Idle:     return QStringLiteral("Idle");
    case State::Charging: return QStringLiteral("Charging");
    case State::Faulted:  return QStringLiteral("Faulted");
    }
    return QStringLiteral("Unknown");
}

QString PileController::stateDisplayText() const
{
    switch (m_state) {
    case State::Booting:  return QStringLiteral("启动中");
    case State::Idle:     return QStringLiteral("空闲");
    case State::Charging: return QStringLiteral("充电中");
    case State::Faulted:  return QStringLiteral("故障");
    }
    return QStringLiteral("未知");
}

bool PileController::isConnected() const
{
    return m_conn && m_conn->isConnected();
}

void PileController::start()
{
    setState(State::Booting);
    m_conn->connectToServer();
}

void PileController::injectDisconnect()
{
    emit logMessage(QStringLiteral("手动断线,将自动重连"));
    m_conn->disconnectFromServer(true);
}

void PileController::injectFault()
{
    if (m_state == State::Faulted)
        return;
    setState(State::Faulted);
    m_powerKw = 0.0;
    m_meterTimer.stop();
    sendStatus();
    emit meterUpdated(m_powerKw, m_energyKwh);
    emit logMessage(QStringLiteral("已注入设备故障,进入故障状态"));
}

void PileController::recoverFromFault()
{
    if (m_state != State::Faulted)
        return;
    setState(State::Idle);
    sendStatus();
    emit logMessage(QStringLiteral("故障已恢复,回到空闲"));
}

void PileController::simulateFullCharge()
{
    if (m_state != State::Charging)
        return;
    m_powerKw = 0.0;
    m_meterTimer.stop();
    setState(State::Idle);
    sendStatus();
    emit meterUpdated(m_powerKw, m_energyKwh);
    emit logMessage(QStringLiteral("本地模拟充满,回到空闲"));
}

void PileController::setState(State state)
{
    if (m_state == state)
        return;
    m_state = state;
    emit stateChanged(m_state);
}

void PileController::onConnected()
{
    emit connectionChanged(true);
    emit logMessage(QStringLiteral("已连接平台,发送开机注册请求"));
    sendBoot();
}

void PileController::onDisconnected()
{
    m_heartbeatTimer.stop();
    m_meterTimer.stop();
    m_powerKw = 0.0;
    setState(State::Booting);
    emit connectionChanged(false);
    emit meterUpdated(m_powerKw, m_energyKwh);
    emit logMessage(QStringLiteral("连接断开"));
}

void PileController::sendBoot()
{
    QJsonObject payload{{QStringLiteral("chargePointVendor"), QStringLiteral("NCS-Sim")},
                        {QStringLiteral("chargePointModel"), m_model},
                        {QStringLiteral("chargePointSerialNumber"), m_pileCode}};
    m_conn->sendRequest(QStringLiteral("BootNotification"), payload,
                        [this](bool ok, const QJsonObject &result) {
                            if (!ok) {
                                emit logMessage(QStringLiteral("开机注册请求失败"));
                                return;
                            }
                            const QString status = result.value(QStringLiteral("status")).toString();
                            if (status.compare(QStringLiteral("Accepted"), Qt::CaseInsensitive) == 0) {
                                setState(State::Idle);
                                m_heartbeatTimer.start();
                                sendStatus();
                                emit logMessage(QStringLiteral("开机注册已接受,进入空闲"));
                            } else {
                                emit logMessage(QStringLiteral("开机注册被拒绝:%1").arg(status));
                            }
                        });
}

void PileController::sendStatus()
{
    if (!m_conn->isConnected())
        return;
    QJsonObject payload{{QStringLiteral("connectorId"), 1},
                        {QStringLiteral("status"), stateText()},
                        {QStringLiteral("timestamp"),
                         QDateTime::currentDateTimeUtc().toString(Qt::ISODate)}};
    m_conn->sendRequest(QStringLiteral("StatusNotification"), payload,
                        [this](bool ok, const QJsonObject &) {
                            if (!ok)
                                emit logMessage(QStringLiteral("状态上报失败"));
                        });
}

void PileController::sendHeartbeat()
{
    if (!m_conn->isConnected() || m_state == State::Booting)
        return;
    m_conn->sendRequest(QStringLiteral("Heartbeat"), QJsonObject{},
                        [this](bool ok, const QJsonObject &) {
                            if (!ok)
                                emit logMessage(QStringLiteral("心跳请求失败"));
                        });
}

void PileController::sendMeter()
{
    if (m_state != State::Charging || !m_conn->isConnected())
        return;

    // 模拟 7kW 上下抖动的功率,并按上报周期累积电量
    const double jitter = (QRandomGenerator::global()->bounded(100) - 50) / 100.0;
    m_powerKw = qMax(0.1, 7.0 + jitter);
    m_energyKwh += m_powerKw * (2.0 / 3600.0);
    emit meterUpdated(m_powerKw, m_energyKwh);

    QJsonObject payload{{QStringLiteral("powerKw"), m_powerKw},
                        {QStringLiteral("energyKwh"), m_energyKwh},
                        {QStringLiteral("timestamp"),
                         QDateTime::currentDateTimeUtc().toString(Qt::ISODate)}};
    m_conn->sendRequest(QStringLiteral("MeterValues"), payload, {});
}

void PileController::onRequest(const QString &action, const QJsonObject &payload,
                               const QString &messageId)
{
    Q_UNUSED(payload);

    if (action == QLatin1String("RemoteStartTransaction")) {
        if (m_state == State::Faulted) {
            m_conn->sendError(messageId, QStringLiteral("Faulted"),
                              QStringLiteral("device is faulted"));
            return;
        }
        if (m_state != State::Idle && m_state != State::Booting) {
            m_conn->sendError(messageId, QStringLiteral("NotIdle"),
                              QStringLiteral("not idle"));
            return;
        }
        setState(State::Charging);
        m_powerKw = 7.0;
        m_meterTimer.start();
        m_conn->sendResponse(messageId,
                             QJsonObject{{QStringLiteral("status"), QStringLiteral("Accepted")}});
        sendStatus();
        emit meterUpdated(m_powerKw, m_energyKwh);
        emit logMessage(QStringLiteral("收到远程开始充电指令,进入充电中"));
        return;
    }

    if (action == QLatin1String("RemoteStopTransaction")) {
        if (m_state != State::Charging) {
            m_conn->sendError(messageId, QStringLiteral("NotCharging"),
                              QStringLiteral("not charging"));
            return;
        }
        m_powerKw = 0.0;
        m_meterTimer.stop();
        setState(State::Idle);
        m_conn->sendResponse(messageId,
                             QJsonObject{{QStringLiteral("status"), QStringLiteral("Accepted")}});
        sendStatus();
        emit meterUpdated(m_powerKw, m_energyKwh);
        emit logMessage(QStringLiteral("收到远程停止充电指令,进入空闲"));
        return;
    }

    if (action == QLatin1String("RemoteReset")) {
        m_powerKw = 0.0;
        m_energyKwh = 0.0;
        m_meterTimer.stop();
        setState(State::Booting);
        m_conn->sendResponse(messageId,
                             QJsonObject{{QStringLiteral("status"), QStringLiteral("Accepted")}});
        emit meterUpdated(m_powerKw, m_energyKwh);
        emit logMessage(QStringLiteral("收到远程重启指令,重新进行开机注册"));
        sendBoot();
        return;
    }

    m_conn->sendError(messageId, QStringLiteral("NotSupported"),
                      QStringLiteral("unknown action"));
}
