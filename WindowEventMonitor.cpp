#include "WindowEventMonitor.h"
#include <QDebug>

// 静态成员初始化
WindowEventMonitor::CallbackData WindowEventMonitor::s_callbackData = {nullptr, nullptr};

WindowEventMonitor::WindowEventMonitor(QObject *parent)
    : QObject(parent)
    , m_targetHwnd(nullptr)
    , m_hook(nullptr)
{
    // 防抖定时器：事件触发后300ms才真正发出信号，避免事件风暴
    m_debounceTimer = new QTimer(this);
    m_debounceTimer->setSingleShot(true);
    m_debounceTimer->setInterval(300);

    connect(m_debounceTimer, &QTimer::timeout, this, [this](){
        emit contentMaybeChanged();
    });
}

WindowEventMonitor::~WindowEventMonitor()
{
    stopMonitoring();
}

void WindowEventMonitor::startMonitoring(HWND hwnd)
{
    if (!hwnd) return;

    // 如果已经在监听，先停止
    if (m_hook) {
        stopMonitoring();
    }

    m_targetHwnd = hwnd;

    // 设置回调数据
    s_callbackData.monitor = this;
    s_callbackData.targetHwnd = hwnd;

    // 获取窗口所属进程ID
    DWORD processId = 0;
    GetWindowThreadProcessId(hwnd, &processId);

    /**
     * SetWinEventHook 监听的事件：
     *
     * EVENT_OBJECT_NAMECHANGE (0x800C): 对象名称变化（可能是窗口标题或文档标题）
     * EVENT_OBJECT_VALUECHANGE (0x800E): 对象值变化（文本内容变化）
     * EVENT_OBJECT_REORDER (0x8004): 对象重新排序
     * EVENT_OBJECT_CONTENTSCROLLED (0x8015): 内容滚动
     *
     * 这些事件覆盖了IE控件内容变化的主要场景
     */
    m_hook = SetWinEventHook(
        EVENT_OBJECT_NAMECHANGE,      // 最小事件
        EVENT_OBJECT_VALUECHANGE,     // 最大事件
        nullptr,                       // 模块句柄（nullptr表示当前进程）
        WinEventProc,                  // 回调函数
        processId,                     // 进程ID（只监听SuperMemo进程）
        0,                             // 线程ID（0表示所有线程）
        WINEVENT_OUTOFCONTEXT         // 异步调用，不注入DLL
    );

    if (m_hook) {
        qDebug() << "WindowEventMonitor: Started monitoring window" << hwnd;
    } else {
        qWarning() << "WindowEventMonitor: Failed to set event hook for window" << hwnd;
    }
}

void WindowEventMonitor::stopMonitoring()
{
    if (m_hook) {
        UnhookWinEvent(m_hook);
        m_hook = nullptr;
        qDebug() << "WindowEventMonitor: Stopped monitoring";
    }

    m_debounceTimer->stop();
    m_targetHwnd = nullptr;

    s_callbackData.monitor = nullptr;
    s_callbackData.targetHwnd = nullptr;
}

// Windows事件回调（静态函数）
void CALLBACK WindowEventMonitor::WinEventProc(
    HWINEVENTHOOK hWinEventHook,
    DWORD event,
    HWND hwnd,
    LONG idObject,
    LONG idChild,
    DWORD dwEventThread,
    DWORD dwmsEventTime)
{
    // 获取监控器实例
    WindowEventMonitor* monitor = s_callbackData.monitor;
    if (!monitor) return;

    HWND targetHwnd = s_callbackData.targetHwnd;
    if (!targetHwnd) return;

    // 检查事件是否来自目标窗口或其子窗口
    HWND checkHwnd = hwnd;
    bool isTargetOrChild = false;

    // 向上遍历窗口层次，检查是否属于目标窗口
    while (checkHwnd) {
        if (checkHwnd == targetHwnd) {
            isTargetOrChild = true;
            break;
        }
        checkHwnd = GetParent(checkHwnd);
    }

    if (!isTargetOrChild) {
        return; // 不是目标窗口的事件，忽略
    }

    // 处理不同类型的事件
    switch (event) {
        case EVENT_OBJECT_NAMECHANGE:
        case EVENT_OBJECT_VALUECHANGE:
        case EVENT_OBJECT_REORDER:
        case EVENT_OBJECT_CONTENTSCROLLED:
            // 内容可能变化，启动防抖定时器
            monitor->m_debounceTimer->start();
            break;

        case EVENT_SYSTEM_FOREGROUND:
            // 窗口激活
            if (hwnd == targetHwnd) {
                emit monitor->windowActivated();
            }
            break;

        case EVENT_OBJECT_FOCUS:
            // 焦点事件（可能表示窗口失活）
            // 这里可以根据需要处理
            break;

        default:
            break;
    }
}
