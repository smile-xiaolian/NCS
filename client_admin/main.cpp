#include <QApplication>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QMessageBox::information(nullptr, "NCS 管理端",
        "管理端程序启动成功！\n"
        "这是 9月1日 的工程骨架占位。");

    return app.exec();
}