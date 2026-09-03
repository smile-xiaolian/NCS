#include <QApplication>
#include <QMessageBox>

#include "core/service/PlatformService.h"
#include "ui/AdminMainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QString error;
    if (!PlatformService::initialize(&error)) {
        QMessageBox::critical(nullptr, QStringLiteral("NCS 管理端"),
                              QStringLiteral("数据库初始化失败：") + error);
        return 1;
    }
    AdminMainWindow window;
    window.show();
    return app.exec();
}