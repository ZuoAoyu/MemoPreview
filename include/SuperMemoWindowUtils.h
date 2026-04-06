#ifndef SUPERMEMOWINDOWUTILS_H
#define SUPERMEMOWINDOWUTILS_H
#include "SuperMemoWindowInfo.h"
#include <QVector>

class SuperMemoWindowUtils {
public:
    // 枚举所有可能的 SuperMemo 窗口
    static QVector<SuperMemoWindowInfo> enumerateAllSuperMemoWindows();

    // 为指定 SuperMemo 进程找到最可能承载正文内容的窗口
    static HWND findBestContentWindow(DWORD processId);
};

#endif // SUPERMEMOWINDOWUTILS_H
