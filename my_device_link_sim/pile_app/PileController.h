#pragma once

#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QUrl>

namespace device_link {
class DeviceConnection;
}

// 桩端业务:状态机 + 调用 core 收发消息。唯一认识"充电桩"词汇的地方之一。
class PileController : public QObject {
    Q_OBJECT
public:
    enum class State { Booting, Idle, Charging, Faulted };
    Q_ENUM(State)

    explicit PileController(const QString &pileCode, const QString &model, const QUrl &serverUrl,
                            QObject *parent = nullptr);

    QString pileCode() const { return m_pileCode; }
    State state() const { return m_state; }
    QString stateText() const;        // 协议与日志使用(英文,如 Idle/Charging)
    QString stateDisplayText() const; // 界面显示使用(中文)
    bool isConnected() const;
    double powerKw() const { return m_powerKw; }
    double energyKwh() const { return m_energyKwh; }

    void start();

    // 故障注入 / 恢复(供 GUI 按钮调用)
    void injectDisconnect();  // 手动断线(自动重连)
    void injectFault();       // 设备故障
    void recoverFromFault();  // 故障恢复
    void simulateFullCharge(); // 本地模拟充满

signals:
    void connectionChanged(bool connected);
    void stateChanged(PileController::State state);
    void meterUpdated(double powerKw, double energyKwh);
    void logMessage(const QString &text);

private:
    void setState(State state);
    void sendBoot();
    void sendStatus();
    void sendHeartbeat();
    void sendMeter();
    void onConnected();
    void onDisconnected();
    void onRequest(const QString &action, const QJsonObject &payload, const QString &messageId);

    QString m_pileCode;
    QString m_model;
    device_link::DeviceConnection *m_conn = nullptr;
    State m_state = State::Booting;
    QTimer m_heartbeatTimer;
    QTimer m_meterTimer;
    double m_powerKw = 0.0;
    double m_energyKwh = 0.0;
};
