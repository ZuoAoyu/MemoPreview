#include "MainWindow.h"

#include <QApplication>
#include <QSharedMemory>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    static const QString SHARED_KEY = "MySoft-AppTitle-UniqueKey";
    QSharedMemory sharedMemory(SHARED_KEY);
    if (!sharedMemory.create(1)) {
        // 尝试创建 1 字节的共享内存，若创建失败，通常因为该键值的共享内存已存在。
        QMessageBox::warning(nullptr, "警告", "本程序已在运行中，请勿重复启动。");
        return 1;
    }

    MainWindow w;

    w.show();
    int ret = a.exec();

    sharedMemory.detach();
    return ret;
}
