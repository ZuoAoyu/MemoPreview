#ifndef SUPERMEMOWINDOWINFO_H
#define SUPERMEMOWINDOWINFO_H

#include <QString>
#include <windows.h>

struct SuperMemoWindowInfo {
    HWND hwnd = nullptr;
    QString title;
    DWORD processId = 0;
    QString processExe;
};

#endif // SUPERMEMOWINDOWINFO_H
