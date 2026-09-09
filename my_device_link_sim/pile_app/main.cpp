#include "MockPlatform.h"
#include "PileController.h"
#include "ui/PileMainWindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QtGlobal>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("pile_app"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("充电桩通信模拟器 - 桩端"));
    parser.addHelpOption();
    QCommandLineOption codeOpt({QStringLiteral("c"), QStringLiteral("code")},
                               QStringLiteral("桩编号"), QStringLiteral("code"),
                               QStringLiteral("PILE-001"));
    QCommandLineOption modelOpt({QStringLiteral("m"), QStringLiteral("model")},
                                QStringLiteral("桩型号"), QStringLiteral("model"),
                                QStringLiteral("AC-7kW"));
    QCommandLineOption urlOpt({QStringLiteral("u"), QStringLiteral("url")},
                              QStringLiteral("平台 WebSocket 地址"), QStringLiteral("url"),
                              QStringLiteral("ws://127.0.0.1:9000"));
    QCommandLineOption demoOpt({QStringLiteral("d"), QStringLiteral("demo")},
                               QStringLiteral("自测/演示模式:进程内内置模拟平台,"
                                              "无需真实 platform_app 即可独立运行"));
    QCommandLineOption mockPortOpt({QStringLiteral("mock-port")},
                                   QStringLiteral("内置模拟平台端口(配合 --demo)"),
                                   QStringLiteral("port"), QStringLiteral("9100"));
    parser.addOption(codeOpt);
    parser.addOption(modelOpt);
    parser.addOption(urlOpt);
    parser.addOption(demoOpt);
    parser.addOption(mockPortOpt);
    parser.process(app);

    const bool demo = parser.isSet(demoOpt);
    const quint16 mockPort = static_cast<quint16>(parser.value(mockPortOpt).toUInt());
    MockPlatform mock;
    const QUrl targetUrl = demo
                               ? QUrl(QStringLiteral("ws://127.0.0.1:%1").arg(mockPort))
                               : QUrl(parser.value(urlOpt));

    auto *controller = new PileController(parser.value(codeOpt), parser.value(modelOpt),
                                          targetUrl);
    PileMainWindow window(controller, demo ? &mock : nullptr);
    window.show();
    if (demo)
        mock.listen(mockPort);
    controller->start();
    return app.exec();
}
