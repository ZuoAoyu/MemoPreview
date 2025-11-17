#ifndef WINDOWEVENTMONITOR_H
#define WINDOWEVENTMONITOR_H

#include <QObject>
#include <QTimer>
#include <windows.h>

/**
 * @brief Windows事件监控器，使用事件钩子替代轮询
 *
 * 监听指定窗口的变化事件（如内容更新、窗口激活等），
 * 当检测到变化时发出信号，实现事件驱动而非定时轮询。
 */
class WindowEventMonitor : public QObject
{
    Q_OBJECT

public:
    explicit WindowEventMonitor(QObject *parent = nullptr);
    ~WindowEventMonitor();

    /**
     * @brief 开始监听指定窗口
     * @param hwnd 要监听的窗口句柄
     */
    void startMonitoring(HWND hwnd);

    /**
     * @brief 停止监听
     */
    void stopMonitoring();

    /**
     * @brief 检查是否正在监听
     */
    bool isMonitoring() const { return m_hook != nullptr; }

signals:
    /**
     * @brief 检测到窗口内容可能发生变化
     */
    void contentMaybeChanged();

    /**
     * @brief 窗口获得焦点
     */
    void windowActivated();

    /**
     * @brief 窗口失去焦点
     */
    void windowDeactivated();

private:
    HWND m_targetHwnd;           // 监听的目标窗口
    HWINEVENTHOOK m_hook;        // 事件钩子句柄
    QTimer* m_debounceTimer;     // 防抖定时器，避免事件风暴

    // 静态回调函数（供Windows API调用）
    static void CALLBACK WinEventProc(
        HWINEVENTHOOK hWinEventHook,
        DWORD event,
        HWND hwnd,
        LONG idObject,
        LONG idChild,
        DWORD dwEventThread,
        DWORD dwmsEventTime
    );

    // 从静态回调中获取实例的辅助结构
    struct CallbackData {
        WindowEventMonitor* monitor;
        HWND targetHwnd;
    };
    static CallbackData s_callbackData;
};

#endif // WINDOWEVENTMONITOR_H
