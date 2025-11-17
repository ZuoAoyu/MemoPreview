#ifndef SUPERMEMOWINDOWUTILS_H
#define SUPERMEMOWINDOWUTILS_H
#include "SuperMemoWindowInfo.h"
#include <QVector>

class SuperMemoWindowUtils {
public:
    // 枚举所有可能的 SuperMemo 窗口
    static QVector<SuperMemoWindowInfo> enumerateAllSuperMemoWindows();
};

#endif // SUPERMEMOWINDOWUTILS_H
