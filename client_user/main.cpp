#include <QApplication>
#include <QMessageBox>
#include "core/service/PlatformService.h"
#include "ui/UserMainWindow.h"

int main(int argc, char **argv)
{
    QApplication a(argc, argv);
    QString errorMsg;

    if (!PlatformService::initialize(&errorMsg))
    {
        QMessageBox::critical(nullptr, "NCS 错误", errorMsg);
        return 1;
    }

    UserMainWindow w;
    w.show();

    return a.exec();
}