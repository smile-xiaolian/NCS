#include "ActionDispatcher.h"
#include "DeviceConnection.h"
#include "MessageFrame.h"
#include "PendingRequestTable.h"

#include <QCoreApplication>
#include <QHostAddress>
#include <QSignalSpy>
#include <QTest>
#include <QVector>
#include <QWebSocket>
#include <QWebSocketServer>

using device_link::DeviceConnection;
using device_link::ActionDispatcher;
using device_link::MessageFrame;
using device_link::PendingRequestTable;

class DeviceLinkCoreTest : public QObject {
    Q_OBJECT
private slots:
    void callRoundTrip();
    void callResultRoundTrip();
    void callErrorRoundTrip();
    void invalidJsonRejected();
    void pendingTimeout();
    void orphanResponseEmitted();
    void echoRequestResponse();
    void concurrentRequestsResolveByMessageId();
    void dispatcherRoutesAndReportsUnhandled();
};

void DeviceLinkCoreTest::callRoundTrip()
{
    MessageFrame in;
    in.type = MessageFrame::Call;
    in.messageId = QStringLiteral("msg-1");
    in.action = QStringLiteral("Echo");
    in.payload = QJsonObject{{QStringLiteral("n"), 42}};

    const auto out = MessageFrame::fromJson(in.toJson());
    QVERIFY(out.has_value());
    QCOMPARE(out->type, MessageFrame::Call);
    QCOMPARE(out->messageId, QStringLiteral("msg-1"));
    QCOMPARE(out->action, QStringLiteral("Echo"));
    QCOMPARE(out->payload.value(QStringLiteral("n")).toInt(), 42);
}

void DeviceLinkCoreTest::callResultRoundTrip()
{
    MessageFrame in;
    in.type = MessageFrame::CallResult;
    in.messageId = QStringLiteral("msg-2");
    in.payload = QJsonObject{{QStringLiteral("ok"), true}};

    const auto out = MessageFrame::fromJson(in.toJson());
    QVERIFY(out.has_value());
    QCOMPARE(out->type, MessageFrame::CallResult);
    QCOMPARE(out->payload.value(QStringLiteral("ok")).toBool(), true);
}

void DeviceLinkCoreTest::callErrorRoundTrip()
{
    MessageFrame in;
    in.type = MessageFrame::CallError;
    in.messageId = QStringLiteral("msg-3");
    in.errorCode = QStringLiteral("NotSupported");
    in.errorDesc = QStringLiteral("nope");

    const auto out = MessageFrame::fromJson(in.toJson());
    QVERIFY(out.has_value());
    QCOMPARE(out->type, MessageFrame::CallError);
    QCOMPARE(out->errorCode, QStringLiteral("NotSupported"));
    QCOMPARE(out->errorDesc, QStringLiteral("nope"));
}

void DeviceLinkCoreTest::invalidJsonRejected()
{
    QVERIFY(!MessageFrame::fromJson(QByteArrayLiteral("{bad}")).has_value());
    QVERIFY(!MessageFrame::fromJson(QByteArrayLiteral("[1,\"x\"]")).has_value());
    QVERIFY(!MessageFrame::fromJson(QByteArrayLiteral("[9,\"x\",\"a\",{}]")).has_value());
    QVERIFY(!MessageFrame::fromJson(QByteArrayLiteral("[2,\"\",\"a\",{}]")).has_value());
}

void DeviceLinkCoreTest::pendingTimeout()
{
    PendingRequestTable table;
    bool called = false;
    QString errCode;
    table.add(QStringLiteral("id"),
              [&](bool ok, const QJsonObject &, const QString &ec, const QString &) {
                  called = true;
                  errCode = ec;
                  QVERIFY(!ok);
              },
              100);
    QTRY_VERIFY_WITH_TIMEOUT(called, 1000);
    QCOMPARE(errCode, QStringLiteral("Timeout"));
}

void DeviceLinkCoreTest::orphanResponseEmitted()
{
    PendingRequestTable table;
    QSignalSpy spy(&table, &PendingRequestTable::orphanResponse);

    QVERIFY(!table.completeSuccess(QStringLiteral("missing"), QJsonObject{}));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toString(), QStringLiteral("missing"));
}

void DeviceLinkCoreTest::echoRequestResponse()
{
    QWebSocketServer server(QStringLiteral("test-server"), QWebSocketServer::NonSecureMode);
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    const QUrl url(QStringLiteral("ws://127.0.0.1:%1").arg(server.serverPort()));

    DeviceConnection conn(url);
    QSignalSpy connSpy(&conn, &DeviceConnection::connected);
    QSignalSpy serverSpy(&server, &QWebSocketServer::newConnection);
    conn.connectToServer();
    // 等待平台侧收到新连接(内部跑事件循环,客户端的异步握手才能完成)
    QVERIFY(serverSpy.wait(3000));
    QWebSocket *serverSock = server.nextPendingConnection();
    QVERIFY(serverSock);
    QVERIFY(connSpy.wait(3000));

    // 服务端收到 Call 后回送 CallResult(echo)
    connect(serverSock, &QWebSocket::textMessageReceived, this,
            [serverSock](const QString &text) {
                const auto frame = MessageFrame::fromJson(text.toUtf8());
                if (!frame || frame->type != MessageFrame::Call)
                    return;
                MessageFrame resp;
                resp.type = MessageFrame::CallResult;
                resp.messageId = frame->messageId;
                resp.payload = QJsonObject{
                    {QStringLiteral("echo"), frame->payload.value(QStringLiteral("n"))}};
                serverSock->sendTextMessage(QString::fromUtf8(resp.toJson()));
            });

    bool called = false;
    conn.sendRequest(QStringLiteral("Echo"), QJsonObject{{QStringLiteral("n"), 42}},
                     [&](bool ok, const QJsonObject &result) {
                         called = true;
                         QVERIFY(ok);
                         QCOMPARE(result.value(QStringLiteral("echo")).toInt(), 42);
                     },
                     3000);
    QTRY_VERIFY_WITH_TIMEOUT(called, 3000);

    serverSock->deleteLater();
}

void DeviceLinkCoreTest::concurrentRequestsResolveByMessageId()
{
    QWebSocketServer server(QStringLiteral("test-server"), QWebSocketServer::NonSecureMode);
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    const QUrl url(QStringLiteral("ws://127.0.0.1:%1").arg(server.serverPort()));

    DeviceConnection conn(url);
    QSignalSpy connSpy(&conn, &DeviceConnection::connected);
    QSignalSpy serverSpy(&server, &QWebSocketServer::newConnection);
    conn.connectToServer();
    QVERIFY(serverSpy.wait(3000));
    QWebSocket *serverSock = server.nextPendingConnection();
    QVERIFY(serverSock);
    QVERIFY(connSpy.wait(3000));

    // 收齐两笔请求后逆序回复:验证每个响应按 messageId 与对应请求配对,不串号
    struct PendingCall {
        QString messageId;
        QString tag;
    };
    QVector<PendingCall> pending;
    connect(serverSock, &QWebSocket::textMessageReceived, this,
            [serverSock, &pending](const QString &text) {
                const auto frame = MessageFrame::fromJson(text.toUtf8());
                if (!frame || frame->type != MessageFrame::Call)
                    return;
                pending.append({frame->messageId,
                                frame->payload.value(QStringLiteral("tag")).toString()});
                if (pending.size() < 2)
                    return;
                for (int i = pending.size() - 1; i >= 0; --i) {
                    MessageFrame resp;
                    resp.type = MessageFrame::CallResult;
                    resp.messageId = pending.at(i).messageId;
                    resp.payload = QJsonObject{{QStringLiteral("tag"), pending.at(i).tag}};
                    serverSock->sendTextMessage(QString::fromUtf8(resp.toJson()));
                }
            });

    bool firstOk = false;
    bool secondOk = false;
    QString firstTag;
    QString secondTag;
    conn.sendRequest(QStringLiteral("Check"),
                     QJsonObject{{QStringLiteral("tag"), QStringLiteral("A")}},
                     [&](bool ok, const QJsonObject &result) {
                         firstOk = ok;
                         firstTag = result.value(QStringLiteral("tag")).toString();
                     },
                     3000);
    conn.sendRequest(QStringLiteral("Check"),
                     QJsonObject{{QStringLiteral("tag"), QStringLiteral("B")}},
                     [&](bool ok, const QJsonObject &result) {
                         secondOk = ok;
                         secondTag = result.value(QStringLiteral("tag")).toString();
                     },
                     3000);
    QTRY_VERIFY_WITH_TIMEOUT(firstOk && secondOk, 3000);
    QCOMPARE(firstTag, QStringLiteral("A"));
    QCOMPARE(secondTag, QStringLiteral("B"));

    serverSock->deleteLater();
}

void DeviceLinkCoreTest::dispatcherRoutesAndReportsUnhandled()
{
    ActionDispatcher dispatcher;
    QSignalSpy spy(&dispatcher, &ActionDispatcher::unhandledAction);

    QString seenAction;
    QString seenId;
    QJsonObject seenPayload;
    dispatcher.registerHandler(
        QStringLiteral("Echo"),
        [&](const QJsonObject &payload, const QString &messageId) {
            seenAction = QStringLiteral("Echo");
            seenId = messageId;
            seenPayload = payload;
        });

    QJsonObject payload{{QStringLiteral("n"), 7}};
    QVERIFY(dispatcher.dispatch(QStringLiteral("Echo"), payload, QStringLiteral("m1")));
    QCOMPARE(seenAction, QStringLiteral("Echo"));
    QCOMPARE(seenId, QStringLiteral("m1"));
    QCOMPARE(seenPayload.value(QStringLiteral("n")).toInt(), 7);

    QVERIFY(!dispatcher.dispatch(QStringLiteral("Unknown"), QJsonObject{}, QStringLiteral("m2")));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toString(), QStringLiteral("Unknown"));
}

QTEST_MAIN(DeviceLinkCoreTest)

#include "test_core.moc"
