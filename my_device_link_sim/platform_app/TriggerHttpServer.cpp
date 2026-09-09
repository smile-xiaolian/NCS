#include "TriggerHttpServer.h"

#include <QHostAddress>
#include <QRegularExpression>
#include <QTcpSocket>
#include <QUrl>
#include <QUrlQuery>

TriggerHttpServer::TriggerHttpServer(QObject *parent)
    : QObject(parent)
{
    connect(&m_server, &QTcpServer::newConnection, this, &TriggerHttpServer::onNewConnection);
}

bool TriggerHttpServer::listen(quint16 port)
{
    if (!m_server.listen(QHostAddress::LocalHost, port)) {
        emit logMessage(QStringLiteral("HTTP 触发口监听失败:%1").arg(m_server.errorString()));
        return false;
    }
    emit logMessage(QStringLiteral("HTTP 触发口 http://127.0.0.1:%1/reset?pileCode=XXX")
                        .arg(m_server.serverPort()));
    return true;
}

quint16 TriggerHttpServer::port() const
{
    return m_server.serverPort();
}

void TriggerHttpServer::onNewConnection()
{
    while (m_server.hasPendingConnections()) {
        QTcpSocket *sock = m_server.nextPendingConnection();
        connect(sock, &QTcpSocket::readyRead, this, [this, sock]() {
            const QByteArray raw = sock->readAll();
            const QString request = QString::fromUtf8(raw);
            const QString firstLine = request.section(QLatin1Char('\n'), 0, 0).trimmed();
            const QStringList parts = firstLine.split(QLatin1Char(' '));
            QString pathAndQuery;
            if (parts.size() >= 2)
                pathAndQuery = parts.at(1);

            QUrl url(pathAndQuery);
            QString pileCode = QUrlQuery(url).queryItemValue(QStringLiteral("pileCode"));
            if (pileCode.isEmpty()) {
                static const QRegularExpression re(QStringLiteral("pileCode=([^&\\s]+)"));
                const auto m = re.match(request);
                if (m.hasMatch())
                    pileCode = m.captured(1);
            }

            QByteArray body;
            int status = 404;
            QString statusText = QStringLiteral("Not Found");
            if (url.path() == QLatin1String("/reset") && !pileCode.isEmpty()) {
                emit resetRequested(pileCode);
                status = 200;
                statusText = QStringLiteral("OK");
                body = QByteArrayLiteral("{\"ok\":true}");
            } else if (url.path() == QLatin1String("/health")) {
                status = 200;
                statusText = QStringLiteral("OK");
                body = QByteArrayLiteral("{\"ok\":true}");
            } else {
                body = QByteArrayLiteral("{\"ok\":false,\"error\":\"use /reset?pileCode=XXX\"}");
            }

            QByteArray resp;
            resp += "HTTP/1.1 " + QByteArray::number(status) + ' ' + statusText.toUtf8() + "\r\n";
            resp += "Content-Type: application/json\r\n";
            resp += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
            resp += "Connection: close\r\n\r\n";
            resp += body;
            sock->write(resp);
            sock->disconnectFromHost();
        });
        connect(sock, &QTcpSocket::disconnected, sock, &QTcpSocket::deleteLater);
    }
}
