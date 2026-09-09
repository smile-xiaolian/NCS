#pragma once

#include <QDateTime>
#include <QHash>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QTimer>

#include <functional>

class ProjectPileDb;
class QWebSocketServer;

namespace device_link {
class DeviceConnection;
class DeviceRegistry;
}

struct PileRuntimeInfo {
    QString pileCode;
    QString model;
    QString stationName; // 项目电站名(命中 NCS 项目电桩时才有值)
    QString status = QStringLiteral("Booting");
    double powerKw = 0.0;
    double energyKwh = 0.0;
    QDateTime lastHeartbeat;
    bool online = true;
};

// 平台端业务:监听端口、维护桩列表、心跳超时判定离线、下发远程指令。
class PlatformController : public QObject {
    Q_OBJECT
public:
    explicit PlatformController(quint16 wsPort = 9000, QObject *parent = nullptr);
    ~PlatformController() override;

    bool start();
    quint16 wsPort() const { return m_wsPort; }
    QList<PileRuntimeInfo> onlinePiles() const;
    QList<PileRuntimeInfo> allPiles() const;
    bool isOnline(const QString &pileCode) const;
    // 协议状态(Idle/Charging/Faulted/Offline/...)转界面中文显示
    static QString statusDisplayText(const QString &wireStatus);

    void remoteStart(const QString &pileCode);
    void remoteStop(const QString &pileCode);
    void remoteReset(const QString &pileCode);
    // 接入 NCS 主工程数据库(可选):登记/状态回写项目充电桩
    void setProjectDb(ProjectPileDb *db);

signals:
    void pilesChanged();
    void logMessage(const QString &text);

private:
    void onNewConnection();
    void bindConnection(device_link::DeviceConnection *conn);
    void onRequest(device_link::DeviceConnection *conn, const QString &action,
                   const QJsonObject &payload, const QString &messageId);
    void checkOfflineDevices();
    void markOffline(const QString &pileCode);
    void syncProjectState(const QString &pileCode, const QString &wireStatus,
                          const QString &action);
    void upsertPile(const QString &pileCode,
                    const std::function<void(PileRuntimeInfo &)> &mutator);

    quint16 m_wsPort;
    QWebSocketServer *m_server = nullptr;
    device_link::DeviceRegistry *m_registry = nullptr;
    QHash<QString, PileRuntimeInfo> m_piles;
    QHash<device_link::DeviceConnection *, QString> m_connToPile;
    QTimer m_offlineTimer;
    ProjectPileDb *m_projectDb = nullptr;
};
