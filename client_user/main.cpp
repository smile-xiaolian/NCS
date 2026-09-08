#include <QApplication>
#include <QMessageBox>
#include <QDirIterator>
#include <QDebug>
#include "core/service/PlatformService.h"
#include "ui/UserMainWindow.h"

int main(int argc, char **argv)
{

    // 1. 彻底禁用沙盒
    qputenv("QTWEBENGINE_DISABLE_SANDBOX", "1");

    // 2. 启用单进程模式 (--single-process)，彻底解决 Linux 虚拟机下 Mojo ChildProcessHost IPC 校验失败 (reason 123) 导致的闪退
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS", 
            "--single-process "
            "--no-sandbox "
            "--disable-gpu "
            "--disable-software-rasterizer "
            "--disable-dev-shm-usage "
            "--disable-seccomp-filter-sandbox");
    
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
