#include "SuperMemoWindowUtils.h"

#include <windows.h>
#include <psapi.h>

static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    QVector<SuperMemoWindowInfo>* result = reinterpret_cast<QVector<SuperMemoWindowInfo>*>(lParam);

    char title[256] = {0};
    ::GetWindowTextA(hwnd, title, sizeof(title)-1);
    char className[128] = {0};
    ::GetClassNameA(hwnd, className, sizeof(className)-1);

    // 筛选 SuperMemo 标识
    // bool isSM = (strstr(title, "SuperMemo") || strstr(className, "TElWind"));
    bool isSM = strstr(className, "TElWind");
    if (!isSM)
        return TRUE;

    // 检查是否可见主窗口
    if (!IsWindowVisible(hwnd))
        return TRUE;

    // 获取进程信息
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);

    QString exePath;
    HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (hProc) {
        WCHAR exeName[512] = {0};
        DWORD size = sizeof(exeName)/sizeof(WCHAR);
        if (GetProcessImageFileNameW(hProc, exeName, size)) {
            exePath = QString::fromWCharArray(exeName);
        }
        CloseHandle(hProc);
    }

    SuperMemoWindowInfo info;
    info.hwnd = hwnd;
    info.title = QString::fromLocal8Bit(title);
    info.processId = pid;
    info.processExe = exePath;
    result->append(info);

    return TRUE;
}

QVector<SuperMemoWindowInfo> SuperMemoWindowUtils::enumerateAllSuperMemoWindows()
{
    QVector<SuperMemoWindowInfo> res;
    // 枚举系统中所有顶层窗口，每找到一个窗口就调用提供的回调函数
    ::EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&res));
    return res;
}
