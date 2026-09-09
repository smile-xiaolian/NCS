#include "DemoPiles.h"
#include "PlatformController.h"
#include "ProjectPileDb.h"
#include "TriggerHttpServer.h"
#include "ui/PlatformMainWindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QMessageBox>
#include <QtGlobal>

namespace {

// 由项目数据库电桩记录生成内置虚拟桩的型号文本(如"快充 120kW")
QString pileModelText(const ProjectPileDb::Charger &c)
{
    return QStringLiteral("%1 %2kW").arg(c.type).arg(c.powerKw, 0, 'f', 0);
}

} // namespace

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("platform_app"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("充电桩通信模拟器 - 平台端"));
    parser.addHelpOption();
    QCommandLineOption wsOpt({QStringLiteral("p"), QStringLiteral("port")},
                             QStringLiteral("WebSocket 端口"), QStringLiteral("port"),
                             QStringLiteral("9000"));
    QCommandLineOption httpOpt({QStringLiteral("http-port")},
                               QStringLiteral("HTTP 触发端口(RemoteReset)"),
                               QStringLiteral("http-port"), QStringLiteral("9080"));
    QCommandLineOption demoOpt({QStringLiteral("d"), QStringLiteral("demo")},
                               QStringLiteral("演示模式:进程内内置虚拟桩,"
                                              "无需启动 pile_app 即可独立测试"));
    QCommandLineOption demoCountOpt({QStringLiteral("demo-piles")},
                                    QStringLiteral("演示模式内置虚拟桩数量(默认 3;项目数据库模式可指定更多)"),
                                    QStringLiteral("数量"), QStringLiteral("3"));
    QCommandLineOption dbOpt({QStringLiteral("db")},
                             QStringLiteral("NCS 主工程数据库(charge_platform.db)路径;"
                                            "不指定时按主工程规则自动查找"),
                             QStringLiteral("path"));
    QCommandLineOption noDbOpt({QStringLiteral("no-db")},
                               QStringLiteral("不连接项目数据库,仅使用独立 DEMO 桩演示"));
    parser.addOption(wsOpt);
    parser.addOption(httpOpt);
    parser.addOption(demoOpt);
    parser.addOption(demoCountOpt);
    parser.addOption(dbOpt);
    parser.addOption(noDbOpt);
    parser.process(app);

    const quint16 wsPort = static_cast<quint16>(parser.value(wsOpt).toUInt());
    const quint16 httpPort = static_cast<quint16>(parser.value(httpOpt).toUInt());

    PlatformController controller(wsPort);
    if (!controller.start()) {
        QMessageBox::critical(nullptr, QStringLiteral("启动失败"),
                              QStringLiteral("无法监听 WebSocket 端口 %1").arg(wsPort));
        return 1;
    }

    TriggerHttpServer http;
    QObject::connect(&http, &TriggerHttpServer::resetRequested, &controller,
                     &PlatformController::remoteReset);
    QObject::connect(&http, &TriggerHttpServer::logMessage, &controller,
                     &PlatformController::logMessage);
    http.listen(httpPort);

    PlatformMainWindow window(&controller, http.port() ? http.port() : httpPort);
    window.show();

    // 连接 NCS 主工程数据库(可选):把项目里登记的充电桩纳入模拟,状态实时回写
    ProjectPileDb projectDb(parser.isSet(dbOpt) ? parser.value(dbOpt)
                                                : ProjectPileDb::defaultDatabasePath());
    if (!parser.isSet(noDbOpt)) {
        QString dbError;
        if (projectDb.open(&dbError))
            controller.setProjectDb(&projectDb);
        else
            qWarning().noquote() << QStringLiteral("platform_app:未连接项目数据库(%1),"
                                                   "本次按独立模式运行")
                                        .arg(dbError);
    }

    if (parser.isSet(demoOpt)) {
        DemoPiles *demo = nullptr;
        // 项目数据库可用时,默认把数据库在册电桩作为内置虚拟桩接入平台
        if (projectDb.isOpen()) {
            QList<DemoPiles::PileSeed> seeds;
            const bool capped = parser.isSet(demoCountOpt);
            const int cap = qBound(1, parser.value(demoCountOpt).toInt(), 200);
            for (const ProjectPileDb::Charger &c : projectDb.chargers()) {
                if (capped && seeds.size() >= cap)
                    break;
                seeds.append(DemoPiles::PileSeed{c.code, pileModelText(c)});
            }
            if (!seeds.isEmpty())
                demo = new DemoPiles(wsPort, seeds, &controller);
        }
        // 无数据库或数据库为空时退回独立 DEMO 桩
        if (!demo) {
            const int demoCount = qBound(1, parser.value(demoCountOpt).toInt(), 8);
            demo = new DemoPiles(wsPort, demoCount, &controller);
        }
        QObject::connect(demo, &DemoPiles::logMessage, &controller,
                         &PlatformController::logMessage);
        window.attachDemoPiles(demo);
        demo->start();
    }

    return app.exec();
}