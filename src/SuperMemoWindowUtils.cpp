#include "SuperMemoWindowUtils.h"

#include "SuperMemoIeExtractor.h"

#include <QFileInfo>
#include <QHash>
#include <QSet>

#include <limits>
#include <windows.h>
#include <psapi.h>

static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam);

namespace {
struct CandidateWindow {
    SuperMemoWindowInfo info;
    QString className;
};

struct CandidateWindowsContext {
    QVector<CandidateWindow>* result = nullptr;
    DWORD processIdFilter = 0;
};

struct WindowContentProbe {
    CandidateWindow window;
    qint64 score = std::numeric_limits<qint64>::min();
    qint64 bestControlScore = 0;
    int nonEmptyControlCount = 0;
    int visibleNonEmptyControlCount = 0;
    qsizetype totalContentLength = 0;
    qint64 largestNonEmptyControlArea = 0;
};

QString queryProcessImagePath(DWORD pid)
{
    QString exePath;
    HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (!hProc) {
        return exePath;
    }

    WCHAR exeName[512] = {0};
    DWORD size = sizeof(exeName) / sizeof(WCHAR);
    if (GetProcessImageFileNameW(hProc, exeName, size)) {
        exePath = QString::fromWCharArray(exeName);
    }
    CloseHandle(hProc);
    return exePath;
}

bool isLikelySuperMemoExecutable(const QString& processExe)
{
    const QString fileName = QFileInfo(processExe).fileName().toLower();
    if (fileName.contains("supermemo")) {
        return true;
    }

    return fileName.startsWith("sm") && fileName.endsWith(".exe");
}

bool hasSuperMemoLikeWindowClass(const QVector<CandidateWindow>& windows)
{
    for (const auto& window : windows) {
        if (window.className.contains(QStringLiteral("TElWind"), Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

bool isTopLevelAppWindow(HWND hwnd)
{
    if (::GetAncestor(hwnd, GA_ROOT) != hwnd) {
        return false;
    }

    const LONG_PTR style = ::GetWindowLongPtr(hwnd, GWL_STYLE);
    if ((style & WS_CHILD) != 0) {
        return false;
    }

    return true;
}

bool isEffectivelyMinimized(HWND hwnd)
{
    if (!hwnd) {
        return false;
    }

    if (IsIconic(hwnd)) {
        return true;
    }

    const HWND rootOwner = GetAncestor(hwnd, GA_ROOTOWNER);
    return rootOwner && rootOwner != hwnd && IsIconic(rootOwner);
}

qint64 windowArea(HWND hwnd)
{
    RECT rect{};
    if (!::GetWindowRect(hwnd, &rect)) {
        return 0;
    }

    const qint64 width = qMax<LONG>(0, rect.right - rect.left);
    const qint64 height = qMax<LONG>(0, rect.bottom - rect.top);
    return width * height;
}

qint64 rectArea(const RECT& rect)
{
    const qint64 width = qMax<LONG>(0, rect.right - rect.left);
    const qint64 height = qMax<LONG>(0, rect.bottom - rect.top);
    return width * height;
}

int mainWindowStructureScore(const CandidateWindow& window)
{
    int score = 0;
    const LONG_PTR style = ::GetWindowLongPtr(window.info.hwnd, GWL_STYLE);
    const LONG_PTR exStyle = ::GetWindowLongPtr(window.info.hwnd, GWL_EXSTYLE);

    if ((style & WS_CAPTION) == WS_CAPTION) {
        score += 4;
    }
    if ((style & WS_SYSMENU) != 0) {
        score += 3;
    }
    if ((style & WS_MINIMIZEBOX) != 0) {
        score += 2;
    }
    if ((style & WS_MAXIMIZEBOX) != 0) {
        score += 2;
    }
    if ((exStyle & WS_EX_APPWINDOW) != 0) {
        score += 2;
    }
    if (!window.info.title.trimmed().isEmpty()) {
        score += 2;
    }
    if (window.className.compare(QStringLiteral("TElWind"), Qt::CaseInsensitive) == 0) {
        score += 3;
    }
    if (!window.info.isMinimized) {
        score += 1;
    }

    const qint64 area = windowArea(window.info.hwnd);
    if (area >= 700000) {
        score += 4;
    } else if (area >= 250000) {
        score += 3;
    } else if (area >= 80000) {
        score += 1;
    }

    return score;
}

QVector<CandidateWindow> enumerateCandidateWindows(DWORD processIdFilter = 0)
{
    QVector<CandidateWindow> enumerated;
    CandidateWindowsContext context{&enumerated, processIdFilter};
    ::EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&context));

    QVector<CandidateWindow> uniqueWindows;
    uniqueWindows.reserve(enumerated.size());
    QSet<quintptr> seenHandles;
    for (const auto& window : enumerated) {
        const quintptr handleValue = reinterpret_cast<quintptr>(window.info.hwnd);
        if (seenHandles.contains(handleValue)) {
            continue;
        }
        seenHandles.insert(handleValue);
        uniqueWindows.append(window);
    }

    if (processIdFilter != 0) {
        const QString exePath = queryProcessImagePath(processIdFilter);
        if (isLikelySuperMemoExecutable(exePath) || hasSuperMemoLikeWindowClass(uniqueWindows)) {
            return uniqueWindows;
        }
        return {};
    }

    QHash<DWORD, QVector<CandidateWindow>> windowsByPid;
    for (const auto& window : uniqueWindows) {
        windowsByPid[window.info.processId].append(window);
    }

    QVector<CandidateWindow> filteredWindows;
    for (auto it = windowsByPid.cbegin(); it != windowsByPid.cend(); ++it) {
        const QString exePath = queryProcessImagePath(it.key());
        if (!isLikelySuperMemoExecutable(exePath) && !hasSuperMemoLikeWindowClass(it.value())) {
            continue;
        }

        for (const auto& window : it.value()) {
            filteredWindows.append(window);
        }
    }

    return filteredWindows;
}

WindowContentProbe probeWindowContent(const CandidateWindow& candidate)
{
    WindowContentProbe probe;
    probe.window = candidate;

    SuperMemoIeExtractor extractor(candidate.info.hwnd);
    const auto controls = extractor.extractAllIeControls();
    const int ieControlCount = static_cast<int>(controls.size());

    for (const auto& ctrl : controls) {
        const QString trimmed = ctrl.content.trimmed();
        if (trimmed.isEmpty()) {
            continue;
        }

        const qint64 controlArea = rectArea(ctrl.rect);
        qint64 controlScore =
            qMin<qint64>(controlArea / 10, 500000000LL) +
            static_cast<qint64>(qMin<qsizetype>(trimmed.size(), 20000)) * 1000LL;
        if (ctrl.isVisible) {
            controlScore += 100000000LL;
        }

        ++probe.nonEmptyControlCount;
        if (ctrl.isVisible) {
            ++probe.visibleNonEmptyControlCount;
        }
        probe.totalContentLength += trimmed.size();
        probe.largestNonEmptyControlArea = qMax(probe.largestNonEmptyControlArea, controlArea);
        probe.bestControlScore = qMax(probe.bestControlScore, controlScore);
    }

    probe.score =
        probe.bestControlScore * 100LL +
        static_cast<qint64>(probe.nonEmptyControlCount) * 10000000LL +
        static_cast<qint64>(probe.visibleNonEmptyControlCount) * 1000000LL +
        static_cast<qint64>(qMin<qsizetype>(probe.totalContentLength, 200000)) * 100LL +
        static_cast<qint64>(ieControlCount) * 100LL +
        mainWindowStructureScore(probe.window);

    return probe;
}

bool isBetterProbe(const WindowContentProbe& candidate, const WindowContentProbe& currentBest)
{
    if (candidate.score != currentBest.score) {
        return candidate.score > currentBest.score;
    }
    if (candidate.bestControlScore != currentBest.bestControlScore) {
        return candidate.bestControlScore > currentBest.bestControlScore;
    }
    if (candidate.nonEmptyControlCount != currentBest.nonEmptyControlCount) {
        return candidate.nonEmptyControlCount > currentBest.nonEmptyControlCount;
    }
    if (candidate.visibleNonEmptyControlCount != currentBest.visibleNonEmptyControlCount) {
        return candidate.visibleNonEmptyControlCount > currentBest.visibleNonEmptyControlCount;
    }
    if (candidate.totalContentLength != currentBest.totalContentLength) {
        return candidate.totalContentLength > currentBest.totalContentLength;
    }
    if (candidate.largestNonEmptyControlArea != currentBest.largestNonEmptyControlArea) {
        return candidate.largestNonEmptyControlArea > currentBest.largestNonEmptyControlArea;
    }
    return windowArea(candidate.window.info.hwnd) > windowArea(currentBest.window.info.hwnd);
}

CandidateWindow selectBestWindow(const QVector<CandidateWindow>& candidates)
{
    WindowContentProbe bestProbe;
    bool hasBest = false;

    for (const auto& candidate : candidates) {
        const WindowContentProbe probe = probeWindowContent(candidate);
        if (!hasBest || isBetterProbe(probe, bestProbe)) {
            bestProbe = probe;
            hasBest = true;
        }
    }

    return hasBest ? bestProbe.window : CandidateWindow{};
}
}

static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    auto* context = reinterpret_cast<CandidateWindowsContext*>(lParam);
    QVector<CandidateWindow>* result = context->result;

    char title[256] = {0};
    ::GetWindowTextA(hwnd, title, sizeof(title)-1);
    char className[128] = {0};
    ::GetClassNameA(hwnd, className, sizeof(className)-1);

    if (!isTopLevelAppWindow(hwnd))
        return TRUE;

    // 获取进程信息
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (context->processIdFilter != 0 && pid != context->processIdFilter) {
        return TRUE;
    }

    CandidateWindow window;
    window.info.hwnd = hwnd;
    window.info.title = QString::fromLocal8Bit(title);
    window.className = QString::fromLocal8Bit(className);
    window.info.processId = pid;
    window.info.isMinimized = isEffectivelyMinimized(hwnd);
    result->append(window);

    return TRUE;
}

QVector<SuperMemoWindowInfo> SuperMemoWindowUtils::enumerateAllSuperMemoWindows()
{
    const QVector<CandidateWindow> candidates = enumerateCandidateWindows();

    QHash<DWORD, QVector<CandidateWindow>> windowsByPid;
    for (const auto& window : candidates) {
        windowsByPid[window.info.processId].append(window);
    }

    QVector<SuperMemoWindowInfo> primaryWindows;
    primaryWindows.reserve(windowsByPid.size());
    for (auto it = windowsByPid.cbegin(); it != windowsByPid.cend(); ++it) {
        const CandidateWindow bestWindow = selectBestWindow(it.value());
        if (bestWindow.info.hwnd) {
            primaryWindows.append(bestWindow.info);
        }
    }

    std::sort(primaryWindows.begin(), primaryWindows.end(), [](const SuperMemoWindowInfo& lhs, const SuperMemoWindowInfo& rhs) {
        if (lhs.isMinimized != rhs.isMinimized) {
            return !lhs.isMinimized;
        }
        if (lhs.processId != rhs.processId) {
            return lhs.processId < rhs.processId;
        }
        return reinterpret_cast<quintptr>(lhs.hwnd) < reinterpret_cast<quintptr>(rhs.hwnd);
    });

    return primaryWindows;
}

HWND SuperMemoWindowUtils::findBestContentWindow(DWORD processId)
{
    if (processId == 0) {
        return nullptr;
    }

    const QVector<CandidateWindow> candidates = enumerateCandidateWindows(processId);
    return selectBestWindow(candidates).info.hwnd;
}
