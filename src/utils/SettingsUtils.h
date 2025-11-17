#ifndef SETTINGSUTILS_H
#define SETTINGSUTILS_H

#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QProcessEnvironment>

class SettingsUtils
{
public:
    // Windows 下查找可执行文件
    static QString findInPath(const QString& exeName);

    // 用于自动初始化默认配置
    static void ensureInitialSettings();
};

#endif // SETTINGSUTILS_H
