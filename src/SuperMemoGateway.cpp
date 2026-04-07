#include "SuperMemoGateway.h"

#include <windows.h>
#include <QStringList>
#include "SuperMemoWindowUtils.h"

namespace {
QString buildContentHash(const std::vector<IeControlContent>& controls)
{
    QStringList contentList;
    contentList.reserve(static_cast<qsizetype>(controls.size()));
    for (const auto& ctrl : controls) {
        contentList << QStringLiteral("%1|%2|%3|%4|%5")
                           .arg(reinterpret_cast<quintptr>(ctrl.hwnd))
                           .arg(ctrl.bodyHtmlHash)
                           .arg(ctrl.bodyHtmlLength)
                           .arg(ctrl.htmlTitle)
                           .arg(ctrl.url);
    }
    return contentList.join(QStringLiteral("||"));
}
}

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

SuperMemoExtractionSnapshot SuperMemoGateway::extractControls(HWND hwnd, const std::vector<IeControlContent>& previousControls) const
{
    SuperMemoExtractionSnapshot snapshot;
    if (!hwnd) {
        snapshot.contentHash.clear();
        return snapshot;
    }

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    HWND extractionTarget = hwnd;
    if (pid != 0) {
        const HWND bestWindow = SuperMemoWindowUtils::findBestContentWindow(pid);
        if (bestWindow) {
            extractionTarget = bestWindow;
        }
    }

    SuperMemoIeExtractor extractor(extractionTarget);
    snapshot.controls = extractor.extractAllIeControls(previousControls);
    snapshot.contentHash = buildContentHash(snapshot.controls);
    return snapshot;
}
