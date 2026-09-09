#pragma once

#include <QObject>
#include <QTcpServer>

// 极简 HTTP 触发口(与主工程可选联动):
//   GET /reset?pileCode=XXX -> 发 resetRequested(pileCode)
//   GET /health             -> 健康检查
class TriggerHttpServer : public QObject {
    Q_OBJECT
public:
    explicit TriggerHttpServer(QObject *parent = nullptr);

    bool listen(quint16 port);
    quint16 port() const;

signals:
    void resetRequested(const QString &pileCode);
    void logMessage(const QString &text);

private:
    void onNewConnection();

    QTcpServer m_server;
};
