#ifndef LATEXMKMANAGER_H
#define LATEXMKMANAGER_H

#include <QObject>
#include <QProcess>
#include <QFileSystemWatcher>
#include <QTimer>

constexpr qint64 MAX_LOG_BUFFER_SIZE = 1024 * 1024; // 1MB

class LatexmkManager : public QObject
{
    Q_OBJECT
public:
    explicit LatexmkManager(QObject *parent = nullptr);
    ~LatexmkManager();

    // 启动/停止latexmk
    void start(const QString &latexmkPath, const QString &workspaceDir);
    void stop();
    void restart(); // 用于崩溃自恢复

    // 状态查询
    bool isRunning() const;

    // 获取全部日志（可用于导出）
    QString fullLog() const;
    // 清空日志缓存
    void clearLog();

signals:
    void compileStatusChanged(const QString &status); // "编译中" "成功" "失败"
    void logUpdated(const QString &log);
    void processCrashed(); // 当latexmk进程崩溃
    void processStopped(); // latexmk进程停止

private slots:
    void handleProcessStarted();
    void handleProcessError(QProcess::ProcessError error);
    void handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void handleReadyReadStandardOutput();
    void handleReadyReadStandardError();

private:
    QProcess *m_process = nullptr;
    QFileSystemWatcher *m_fileWatcher = nullptr;
    QString m_workspaceDir;
    QString m_latexmkPath;
    // latexmk进程的崩溃是否是由用户主动停止的。主动 stop 时不重启，只有真正崩溃才重启。
    bool m_userStopping = false;

    QByteArray m_logBuffer;

    void emitStatus(const QString &status);
    void startLatexmk();
    void setupConnections();

    // 裁剪日志
    void trimLogBufferIfNeeded();
};

#endif // LATEXMKMANAGER_H
