#include "PlatformController.h"

#include "DeviceConnection.h"
#include "DeviceRegistry.h"
#include "ProjectPileDb.h"

#include <QHostAddress>
#include <QWebSocket>
#include <QWebSocketServer>

#include <algorithm>
#include <functional>

using device_link::DeviceConnection;
using device_link::DeviceRegistry;

namespace {
// 协议状态 -> 主工程 charger.status(0 空闲 / 1 充电中 / 2 故障)
int projectDbStatusFor(const QString &wire)
{
    if (wire == QLatin1String("Charging"))
        return 1;
    if (wire == QLatin1String("Faulted"))
        return 2;
    return 0;
}
} // namespace

PlatformController::PlatformController(quint16 wsPort, QObject *parent)
    : QObject(parent)
    , m_wsPort(wsPort)
    , m_server(new QWebSocketServer(QStringLiteral("PlatformSimulator"),
                                    QWebSocketServer::NonSecureMode, this))
    , m_registry(new DeviceRegistry(this))
{
    connect(m_server, &QWebSocketServer::newConnection, this, &PlatformController::onNewConnection);

    // 每 5 秒扫一遍心跳,超过 30 秒(3 个心跳周期)判定离线
    m_offlineTimer.setInterval(5000);
    connect(&m_offlineTimer, &QTimer::timeout, this, &PlatformController::checkOfflineDevices);
}

PlatformController::~PlatformController() = default;

bool PlatformController::start()
{
    if (!m_server->listen(QHostAddress::Any, m_wsPort)) {
        emit logMessage(QStringLiteral("监听失败:%1").arg(m_server->errorString()));
        return false;
    }
    m_offlineTimer.start();
    emit logMessage(QStringLiteral("WebSocket 已监听 ws://0.0.0.0:%1").arg(m_wsPort));
    return true;
}

QList<PileRuntimeInfo> PlatformController::onlinePiles() const
{
    QList<PileRuntimeInfo> list = allPiles();
    for (auto it = list.begin(); it != list.end();) {
        if (!it->online)
            it = list.erase(it);
        else
            ++it;
    }
    return list;
}

QList<PileRuntimeInfo> PlatformController::allPiles() const
{
    QList<PileRuntimeInfo> list;
    for (const auto &info : m_piles)
        list.append(info);
    std::sort(list.begin(), list.end(), [](const PileRuntimeInfo &a, const PileRuntimeInfo &b) {
        return a.pileCode < b.pileCode;
    });
    return list;
}

bool PlatformController::isOnline(const QString &pileCode) const
{
    const auto it = m_piles.constFind(pileCode);
    return it != m_piles.constEnd() && it->online;
}

QString PlatformController::statusDisplayText(const QString &wireStatus)
{
    if (wireStatus == QLatin1String("Idle"))
        return QStringLiteral("空闲");
    if (wireStatus == QLatin1String("Charging"))
        return QStringLiteral("充电中");
    if (wireStatus == QLatin1String("Faulted"))
        return QStringLiteral("故障");
    if (wireStatus == QLatin1String("Offline"))
        return QStringLiteral("离线");
    if (wireStatus == QLatin1String("Booting"))
        return QStringLiteral("启动中");
    return wireStatus;
}

void PlatformController::onNewConnection()
{
    while (m_server->hasPendingConnections()) {
        QWebSocket *sock = m_server->nextPendingConnection();
        auto *conn = new DeviceConnection(sock, this);
        bindConnection(conn);
        emit logMessage(QStringLiteral("新设备接入(待开机注册)"));
    }
}

void PlatformController::bindConnection(DeviceConnection *conn)
{
    connect(conn, &DeviceConnection::requestReceived, this,
            [this, conn](const QString &action, const QJsonObject &payload, const QString &id) {
                onRequest(conn, action, payload, id);
            });
    connect(conn, &DeviceConnection::disconnected, this, [this, conn]() {
        const QString pile = m_connToPile.value(conn);
        m_connToPile.remove(conn);
        if (!pile.isEmpty()) {
            m_registry->remove(pile);
            markOffline(pile);
            emit logMessage(QStringLiteral("桩 %1 连接断开").arg(pile));
            emit pilesChanged();
        }
        conn->deleteLater();
    });
    connect(conn, &DeviceConnection::orphanResponse, this, [this](const QString &id) {
        emit logMessage(QStringLiteral("警告:无匹配请求的响应 %1").arg(id));
    });
}

void PlatformController::onRequest(DeviceConnection *conn, const QString &action,
                                   const QJsonObject &payload, const QString &messageId)
{
    if (action == QLatin1String("BootNotification")) {
        const QString pileCode =
            payload.value(QStringLiteral("chargePointSerialNumber")).toString();
        const QString model = payload.value(QStringLiteral("chargePointModel")).toString();
        if (pileCode.isEmpty()) {
            conn->sendError(messageId, QStringLiteral("FormationViolation"),
                            QStringLiteral("missing serial"));
            return;
        }

        // 同一桩编号重复连接时顶掉旧连接
        if (DeviceConnection *old = m_registry->get(pileCode); old && old != conn) {
            m_connToPile.remove(old);
            old->disconnectFromServer(false);
            old->deleteLater();
            m_registry->remove(pileCode);
        }

        m_registry->add(pileCode, conn);
        m_connToPile.insert(conn, pileCode);
        upsertPile(pileCode, [&](PileRuntimeInfo &info) {
            info.pileCode = pileCode;
            info.model = model;
            info.status = QStringLiteral("Idle");
            info.online = true;
            info.lastHeartbeat = QDateTime::currentDateTime();
            info.powerKw = 0.0;
            // 命中 NCS 项目电桩时,以数据库登记的电站/类型/额定功率为准展示
            if (m_projectDb) {
                ProjectPileDb::Charger rec;
                if (m_projectDb->findByCode(pileCode, &rec)) {
                    info.stationName = rec.stationName;
                    info.model = QStringLiteral("%1 %2kW")
                                     .arg(rec.type)
                                     .arg(rec.powerKw, 0, 'f', 0);
                }
            }
        });
        conn->sendResponse(messageId,
                           QJsonObject{{QStringLiteral("status"), QStringLiteral("Accepted")},
                                       {QStringLiteral("currentTime"),
                                        QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},
                                       {QStringLiteral("interval"), 10}});
        syncProjectState(pileCode, QStringLiteral("Idle"), QStringLiteral("上线"));
        emit logMessage(QStringLiteral("桩 %1 上线注册成功").arg(pileCode));
        emit pilesChanged();
        return;
    }

    const QString pileCode = m_connToPile.value(conn);
    if (pileCode.isEmpty()) {
        conn->sendError(messageId, QStringLiteral("NotBooted"), QStringLiteral("boot first"));
        return;
    }

    // 任何消息都视为活跃,刷新最后心跳时间
    upsertPile(pileCode, [](PileRuntimeInfo &info) {
        info.lastHeartbeat = QDateTime::currentDateTime();
        info.online = true;
    });

    if (action == QLatin1String("Heartbeat")) {
        conn->sendResponse(messageId,
                           QJsonObject{{QStringLiteral("currentTime"),
                                        QDateTime::currentDateTimeUtc().toString(Qt::ISODate)}});
        emit pilesChanged();
        return;
    }

    if (action == QLatin1String("StatusNotification")) {
        const QString status = payload.value(QStringLiteral("status")).toString();
        QString previous;
        const auto prev = m_piles.constFind(pileCode);
        if (prev != m_piles.constEnd())
            previous = prev->status;
        upsertPile(pileCode, [&](PileRuntimeInfo &info) {
            info.status = status;
            if (status != QLatin1String("Charging"))
                info.powerKw = 0.0;
        });
        // 状态切换时同步项目数据库(空闲/充电/故障);状态未变(如开机后的首次 Idle 上报)跳过
        if (previous != status) {
            QString dbAction;
            if (status == QLatin1String("Charging"))
                dbAction = QStringLiteral("开始充电");
            else if (status == QLatin1String("Faulted"))
                dbAction = QStringLiteral("故障");
            else if (status == QLatin1String("Idle")) {
                if (previous == QLatin1String("Charging"))
                    dbAction = QStringLiteral("结束充电");
                else if (previous == QLatin1String("Faulted"))
                    dbAction = QStringLiteral("故障恢复");
                else
                    dbAction = QStringLiteral("空闲");
            }
            if (!dbAction.isEmpty())
                syncProjectState(pileCode, status, dbAction);
        }
        conn->sendResponse(messageId, QJsonObject{});
        emit logMessage(QStringLiteral("桩 %1 状态 -> %2")
                            .arg(pileCode, statusDisplayText(status)));
        emit pilesChanged();
        return;
    }

    if (action == QLatin1String("MeterValues")) {
        upsertPile(pileCode, [&](PileRuntimeInfo &info) {
            info.powerKw = payload.value(QStringLiteral("powerKw")).toDouble();
            info.energyKwh = payload.value(QStringLiteral("energyKwh")).toDouble();
            info.status = QStringLiteral("Charging");
        });
        conn->sendResponse(messageId, QJsonObject{});
        emit pilesChanged();
        return;
    }

    conn->sendError(messageId, QStringLiteral("NotSupported"), QStringLiteral("unknown action"));
}

namespace {
// 平台下发指令的回调结果汇总为中文:超时/错误=失败;Accepted=已接受;其余带原因。
QString commandResultText(bool ok, const QJsonObject &result)
{
    if (!ok)
        return QStringLiteral("失败");
    const QString status = result.value(QStringLiteral("status")).toString();
    if (status.compare(QStringLiteral("Accepted"), Qt::CaseInsensitive) == 0)
        return QStringLiteral("已接受");
    return QStringLiteral("已拒绝(%1)").arg(status);
}
} // namespace

void PlatformController::remoteStart(const QString &pileCode)
{
    DeviceConnection *conn = m_registry->get(pileCode);
    if (!conn) {
        emit logMessage(QStringLiteral("下发开始充电失败:%1 不在线").arg(pileCode));
        return;
    }
    conn->sendRequest(QStringLiteral("RemoteStartTransaction"),
                      QJsonObject{{QStringLiteral("connectorId"), 1}},
                      [this, pileCode](bool ok, const QJsonObject &result) {
                          emit logMessage(QStringLiteral("开始充电指令 %1:%2")
                                              .arg(pileCode, commandResultText(ok, result)));
                      });
}

void PlatformController::remoteStop(const QString &pileCode)
{
    DeviceConnection *conn = m_registry->get(pileCode);
    if (!conn) {
        emit logMessage(QStringLiteral("下发停止充电失败:%1 不在线").arg(pileCode));
        return;
    }
    conn->sendRequest(QStringLiteral("RemoteStopTransaction"), QJsonObject{},
                      [this, pileCode](bool ok, const QJsonObject &result) {
                          emit logMessage(QStringLiteral("停止充电指令 %1:%2")
                                              .arg(pileCode, commandResultText(ok, result)));
                      });
}

void PlatformController::remoteReset(const QString &pileCode)
{
    DeviceConnection *conn = m_registry->get(pileCode);
    if (!conn) {
        emit logMessage(QStringLiteral("远程重启失败:%1 不在线").arg(pileCode));
        return;
    }
    conn->sendRequest(QStringLiteral("RemoteReset"),
                      QJsonObject{{QStringLiteral("type"), QStringLiteral("Hard")}},
                      [this, pileCode](bool ok, const QJsonObject &result) {
                          // 桩端重启会清零计量,平台侧同步清零,保持两端显示一致
                          if (ok) {
                              upsertPile(pileCode, [](PileRuntimeInfo &info) {
                                  info.powerKw = 0.0;
                                  info.energyKwh = 0.0;
                              });
                              emit pilesChanged();
                          }
                          emit logMessage(QStringLiteral("远程重启指令 %1:%2")
                                              .arg(pileCode, commandResultText(ok, result)));
                      });
}

void PlatformController::setProjectDb(ProjectPileDb *db)
{
    m_projectDb = db;
    if (db)
        emit logMessage(QStringLiteral("已连接 NCS 项目数据库:%1").arg(db->databasePath()));
}

void PlatformController::syncProjectState(const QString &pileCode, const QString &wireStatus,
                                          const QString &action)
{
    if (!m_projectDb)
        return;
    const int dbStatus = projectDbStatusFor(wireStatus);
    if (!m_projectDb->updateState(pileCode, dbStatus, action))
        return; // 非项目在册电桩(如独立 DEMO 桩),无需回写
    emit logMessage(QStringLiteral("已回写项目数据库:桩 %1 -> %2")
                        .arg(pileCode, action));
}

void PlatformController::checkOfflineDevices()
{
    const QDateTime now = QDateTime::currentDateTime();
    QStringList offline;
    for (auto it = m_piles.begin(); it != m_piles.end(); ++it) {
        if (!it->online)
            continue;
        if (!it->lastHeartbeat.isValid() || it->lastHeartbeat.secsTo(now) > 30)
            offline.append(it.key());
    }
    for (const QString &code : offline) {
        markOffline(code);
        if (DeviceConnection *conn = m_registry->get(code)) {
            m_connToPile.remove(conn);
            m_registry->remove(code);
            conn->disconnectFromServer(false);
            conn->deleteLater();
        }
        emit logMessage(QStringLiteral("桩 %1 心跳超时,判定离线").arg(code));
    }
    if (!offline.isEmpty())
        emit pilesChanged();
}

void PlatformController::markOffline(const QString &pileCode)
{
    auto it = m_piles.find(pileCode);
    if (it == m_piles.end())
        return;
    const bool wasFaulted = it->status == QLatin1String("Faulted");
    it->online = false;
    it->powerKw = 0.0;
    it->status = QStringLiteral("Offline");
    // 故障桩断线保留故障状态;其余回写为空闲(0),并记录"下线"
    syncProjectState(pileCode,
                     wasFaulted ? QStringLiteral("Faulted") : QStringLiteral("Idle"),
                     QStringLiteral("下线"));
}

void PlatformController::upsertPile(const QString &pileCode,
                                    const std::function<void(PileRuntimeInfo &)> &mutator)
{
    auto it = m_piles.find(pileCode);
    if (it == m_piles.end()) {
        PileRuntimeInfo info;
        info.pileCode = pileCode;
        mutator(info);
        m_piles.insert(pileCode, info);
    } else {
        mutator(it.value());
    }
}
