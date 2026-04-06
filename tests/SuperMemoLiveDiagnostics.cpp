#include <QCoreApplication>
#include <QFileInfo>
#include <QHash>
#include <QTextStream>
#include <QVector>

#include <windows.h>
#include <psapi.h>

#include "SuperMemoGateway.h"
#include "SuperMemoIeExtractor.h"
#include "SuperMemoWindowUtils.h"

namespace {
struct TopLevelWindowSnapshot {
    HWND hwnd = nullptr;
    DWORD processId = 0;
    QString processExe;
    QString title;
    QString className;
    HWND owner = nullptr;
    HWND root = nullptr;
    bool isVisible = false;
    bool isMinimized = false;
    LONG_PTR style = 0;
    LONG_PTR exStyle = 0;
    RECT rect{};
    std::vector<IeControlContent> controls;
};

struct EnumWindowsContext {
    QVector<TopLevelWindowSnapshot>* windows = nullptr;
};

QString toHex(quintptr value)
{
    return QStringLiteral("0x%1").arg(value, 0, 16);
}

QString hwndToString(HWND hwnd)
{
    return toHex(reinterpret_cast<quintptr>(hwnd));
}

QString queryProcessImagePath(DWORD pid)
{
    QString exePath;
    HANDLE processHandle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (!processHandle) {
        return exePath;
    }

    WCHAR exeName[512] = {0};
    DWORD size = sizeof(exeName) / sizeof(WCHAR);
    if (GetProcessImageFileNameW(processHandle, exeName, size)) {
        exePath = QString::fromWCharArray(exeName);
    }

    CloseHandle(processHandle);
    return exePath;
}

bool isLikelySuperMemoProcess(const QString& processExe)
{
    const QString fileName = QFileInfo(processExe).fileName().toLower();
    if (fileName.contains(QStringLiteral("supermemo"))) {
        return true;
    }
    return fileName.startsWith(QStringLiteral("sm")) && fileName.endsWith(QStringLiteral(".exe"));
}

QString rectToString(const RECT& rect)
{
    const int width = rect.right - rect.left;
    const int height = rect.bottom - rect.top;
    return QStringLiteral("(%1,%2)-(%3,%4) %5x%6")
        .arg(rect.left)
        .arg(rect.top)
        .arg(rect.right)
        .arg(rect.bottom)
        .arg(width)
        .arg(height);
}

QString summarizeContent(const QString& text)
{
    QString summary = text;
    summary.replace("\r", "\\r");
    summary.replace("\n", "\\n");
    if (summary.size() > 120) {
        summary = summary.left(117) + QStringLiteral("...");
    }
    return summary;
}

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    auto* context = reinterpret_cast<EnumWindowsContext*>(lParam);

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    const QString processExe = queryProcessImagePath(pid);
    if (!isLikelySuperMemoProcess(processExe)) {
        return TRUE;
    }

    char title[512] = {0};
    char className[256] = {0};
    GetWindowTextA(hwnd, title, sizeof(title) - 1);
    GetClassNameA(hwnd, className, sizeof(className) - 1);

    TopLevelWindowSnapshot snapshot;
    snapshot.hwnd = hwnd;
    snapshot.processId = pid;
    snapshot.processExe = processExe;
    snapshot.title = QString::fromLocal8Bit(title);
    snapshot.className = QString::fromLocal8Bit(className);
    snapshot.owner = GetWindow(hwnd, GW_OWNER);
    snapshot.root = GetAncestor(hwnd, GA_ROOT);
    snapshot.isVisible = IsWindowVisible(hwnd);
    snapshot.isMinimized = IsIconic(hwnd);
    snapshot.style = GetWindowLongPtr(hwnd, GWL_STYLE);
    snapshot.exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
    GetWindowRect(hwnd, &snapshot.rect);

    SuperMemoIeExtractor extractor(hwnd);
    snapshot.controls = extractor.extractAllIeControls();

    context->windows->append(std::move(snapshot));
    return TRUE;
}

qint64 rectArea(const RECT& rect)
{
    const qint64 width = qMax<LONG>(0, rect.right - rect.left);
    const qint64 height = qMax<LONG>(0, rect.bottom - rect.top);
    return width * height;
}

void printWindowReport(QTextStream& out, const TopLevelWindowSnapshot& window)
{
    int nonEmptyControls = 0;
    qsizetype totalContentLength = 0;
    qint64 maxControlArea = 0;
    for (const auto& control : window.controls) {
        const QString trimmed = control.content.trimmed();
        if (trimmed.isEmpty()) {
            continue;
        }
        ++nonEmptyControls;
        totalContentLength += trimmed.size();
        maxControlArea = qMax(maxControlArea, rectArea(control.rect));
    }

    out << "Window " << hwndToString(window.hwnd)
        << " pid=" << window.processId
        << " class=\"" << window.className << "\""
        << " title=\"" << window.title << "\""
        << "\n";
    out << "  exe=" << window.processExe << "\n";
    out << "  visible=" << (window.isVisible ? "true" : "false")
        << " iconic=" << (window.isMinimized ? "true" : "false")
        << " owner=" << hwndToString(window.owner)
        << " root=" << hwndToString(window.root)
        << "\n";
    out << "  style=" << toHex(static_cast<quintptr>(window.style))
        << " exStyle=" << toHex(static_cast<quintptr>(window.exStyle))
        << " rect=" << rectToString(window.rect)
        << "\n";
    out << "  ieControls=" << window.controls.size()
        << " nonEmptyControls=" << nonEmptyControls
        << " totalContentLength=" << totalContentLength
        << " maxControlArea=" << maxControlArea
        << "\n";

    for (int i = 0; i < static_cast<int>(window.controls.size()); ++i) {
        const auto& control = window.controls[static_cast<size_t>(i)];
        out << "    Control[" << i << "] hwnd=" << hwndToString(control.hwnd)
            << " visible=" << (control.isVisible ? "true" : "false")
            << " rect=" << rectToString(control.rect)
            << " title=\"" << control.htmlTitle << "\""
            << " url=\"" << control.url << "\""
            << " contentLength=" << control.content.trimmed().size()
            << "\n";
        out << "      sample=\"" << summarizeContent(control.content) << "\"\n";
    }
}
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);

    QVector<TopLevelWindowSnapshot> windows;
    EnumWindowsContext context{&windows};
    EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&context));

    if (windows.isEmpty()) {
        out << "No SuperMemo top-level windows detected.\n";
        return 1;
    }

    QHash<DWORD, QVector<TopLevelWindowSnapshot>> windowsByPid;
    for (const auto& window : windows) {
        windowsByPid[window.processId].append(window);
    }

    const QVector<SuperMemoWindowInfo> productionWindows = SuperMemoWindowUtils::enumerateAllSuperMemoWindows();
    SuperMemoGateway gateway;

    for (auto it = windowsByPid.cbegin(); it != windowsByPid.cend(); ++it) {
        const DWORD pid = it.key();
        out << "==== PID " << pid << " ====\n";
        out << "Process: " << it.value().first().processExe << "\n";
        out << "Current production best hwnd: " << hwndToString(SuperMemoWindowUtils::findBestContentWindow(pid)) << "\n";

        for (const auto& info : productionWindows) {
            if (info.processId != pid) {
                continue;
            }
            out << "Current production enumerateWindows hwnd: " << hwndToString(info.hwnd)
                << " title=\"" << info.title << "\" minimized=" << (info.isMinimized ? "true" : "false")
                << "\n";
            const auto snapshot = gateway.extractControls(info.hwnd);
            out << "  gateway.extractControls => controls=" << snapshot.controls.size()
                << " contentHashLength=" << snapshot.contentHash.size()
                << "\n";
        }

        for (const auto& window : it.value()) {
            printWindowReport(out, window);
        }
    }

    return 0;
}
