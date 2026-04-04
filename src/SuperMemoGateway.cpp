#include "SuperMemoGateway.h"

#include <windows.h>
#include <QStringList>
#include "SuperMemoWindowUtils.h"

QVector<SuperMemoWindowInfo> SuperMemoGateway::enumerateWindows() const
{
    return SuperMemoWindowUtils::enumerateAllSuperMemoWindows();
}

bool SuperMemoGateway::isForegroundProcess(HWND hwnd) const
{
    if (!hwnd) {
        return false;
    }

    HWND foregroundWindow = GetForegroundWindow();
    DWORD foregroundPid = 0;
    DWORD superMemoPid = 0;
    if (foregroundWindow) {
        GetWindowThreadProcessId(foregroundWindow, &foregroundPid);
    }
    GetWindowThreadProcessId(hwnd, &superMemoPid);
    return foregroundPid != 0 && foregroundPid == superMemoPid;
}

SuperMemoExtractionSnapshot SuperMemoGateway::extractControls(HWND hwnd) const
{
    SuperMemoExtractionSnapshot snapshot;
    SuperMemoIeExtractor extractor(hwnd);
    snapshot.controls = extractor.extractAllIeControls();

    QStringList contentList;
    contentList.reserve(static_cast<qsizetype>(snapshot.controls.size()));
    for (const auto& ctrl : snapshot.controls) {
        contentList << ctrl.content;
    }
    snapshot.contentHash = contentList.join("");
    return snapshot;
}
