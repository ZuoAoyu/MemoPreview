#include "LatexmkManager.h"
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QCoreApplication>
#include <windows.h>

LatexmkManager::LatexmkManager(QObject *parent)
{
    m_process = new QProcess(this);
    setupConnections();
}

LatexmkManager::~LatexmkManager()
{
    stop();
#ifdef Q_OS_WIN
    closeJobObject();
#endif
}

void LatexmkManager::start(const QString &latexmkPath, const QString &workspaceDir)
{
    stop();
    m_userStopping = false; // 重置标志

    m_workspaceDir = workspaceDir;
    m_latexmkPath = latexmkPath;

    emitStatus("编译中");
    m_logBuffer.clear();

    startLatexmk();
}

void LatexmkManager::stop()
{
    m_userStopping = true;
    if (m_process->state() == QProcess::NotRunning) {
        return;
    }

    emit logUpdated("停止 latexmk 编译...");
    // m_process->kill();

#ifdef Q_OS_WIN
    // 杀掉整个 Job Object 的所有进程树
    closeJobObject(); // 这会终止 Job Object 下所有子进程
#endif
}

void LatexmkManager::restart()
{
    m_userStopping = false; // 重启视为不是用户主动 stop
    stop();
    QTimer::singleShot(1000, [this]() {
        emit logUpdated("latexmk崩溃，自动重启...");
        startLatexmk();
    });
}

bool LatexmkManager::isRunning() const
{
    return m_process->state() == QProcess::Running;
}

QString LatexmkManager::fullLog() const
{
    return QString::fromLocal8Bit(m_logBuffer);
}

void LatexmkManager::clearLog()
{
    m_logBuffer.clear();
}

void LatexmkManager::handleProcessStarted()
{
    emitStatus("编译中");
}

void LatexmkManager::handleProcessError(QProcess::ProcessError error)
{
    Q_UNUSED(error)
    emitStatus("latexmk进程错误");

    // 只有不是用户主动 stop 时才自动重启
    if (!m_userStopping) {
        emit logUpdated("latexmk 进程出错，准备重启...");
        emit processCrashed();
        // 自动重启由主窗口处理
    }
}

void LatexmkManager::handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::CrashExit && !m_userStopping) {
        // 只有非用户主动停止时才认为是崩溃
        emitStatus("latexmk崩溃");
        emit logUpdated("latexmk崩溃，准备重启...");
        emit processCrashed();
    } else if (exitStatus == QProcess::CrashExit && m_userStopping) {
        // 用户主动停止时的"崩溃"是正常的
        emitStatus("latexmk已停止");
        emit logUpdated("latexmk已被用户停止");
        emit processStopped();
    } else {
        emitStatus("latexmk已退出");
        if (exitCode != 0 && !m_userStopping) {
            emit logUpdated(QString("latexmk退出，退出码: %1").arg(exitCode));
        }
        emit processStopped();
    }

    // 重置停止标志
    m_userStopping = false;
}

void LatexmkManager::handleReadyReadStandardOutput()
{
    QByteArray out = m_process->readAllStandardOutput();
    m_logBuffer.append(out);
    trimLogBufferIfNeeded(); // 裁剪日志避免无限膨胀
    emit logUpdated(QString::fromLocal8Bit(out));
    // 可根据内容自动检测“成功”、“失败”
    if (QString(out).contains("Output written on")) {
        emitStatus("成功");
    } else if (QString(out).contains("LaTeX Error")) {
        emitStatus("失败");
    }
}

void LatexmkManager::handleReadyReadStandardError()
{
    QByteArray err = m_process->readAllStandardError();
    m_logBuffer.append(err);
    trimLogBufferIfNeeded(); // 裁剪日志避免无限膨胀
    emit logUpdated(QString::fromLocal8Bit(err));
    if (QString(err).contains("LaTeX Error")) {
        emitStatus("失败");
    }
}

void LatexmkManager::emitStatus(const QString &status)
{
    emit compileStatusChanged(status);
}

void LatexmkManager::startLatexmk()
{
    // `-view=none`的作用是禁止自动预览，例如禁止自动打开 Sumatra PDF
    QStringList args = {"-pdf", "-pvc", "-outdir=build", "main.tex"};
    m_process->setWorkingDirectory(m_workspaceDir);
    m_process->start(m_latexmkPath, args);

    if (!m_process->waitForStarted(3000)) {
        emitStatus("启动latexmk失败");
        emit logUpdated("无法启动latexmk");
        return;
    }
#ifdef Q_OS_WIN
    assignProcessToJobObject(); // 分配 Job Object
#endif
}

void LatexmkManager::setupConnections()
{
    connect(m_process, &QProcess::started, this, &LatexmkManager::handleProcessStarted);
    connect(m_process, QOverload<QProcess::ProcessError>::of(&QProcess::errorOccurred), this, &LatexmkManager::handleProcessError);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &LatexmkManager::handleProcessFinished);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &LatexmkManager::handleReadyReadStandardOutput);
    connect(m_process, &QProcess::readyReadStandardError, this, &LatexmkManager::handleReadyReadStandardError);
}

void LatexmkManager::trimLogBufferIfNeeded()
{
    if (m_logBuffer.size() > MAX_LOG_BUFFER_SIZE) {
        // 保留最后 N 字节
        m_logBuffer = m_logBuffer.right(MAX_LOG_BUFFER_SIZE);
    }
}

// -------  Job Object相关函数  -------

#ifdef Q_OS_WIN

void LatexmkManager::assignProcessToJobObject()
{
    // 1. 创建 Job Object（如未创建）
    if (m_hJob) {
        // 已有，先清掉
        closeJobObject();
    }

    m_hJob = CreateJobObjectW(nullptr, nullptr);
    if (!m_hJob) {
        emit logUpdated("无法创建 Job Object");
        return;
    }

    // 2. 设置 Job Object 终止时杀掉所有进程
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION jobInfo = {0};
    jobInfo.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (!SetInformationJobObject(m_hJob, JobObjectExtendedLimitInformation, &jobInfo, sizeof(jobInfo))) {
        emit logUpdated("无法设置 Job Object 属性");
        CloseHandle(m_hJob);
        m_hJob = nullptr;
        return;
    }

    // 3. 获取子进程句柄
    qint64 pid = m_process->processId();
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, DWORD(pid));
    if (!hProcess) {
        emit logUpdated("无法打开子进程句柄");
        CloseHandle(m_hJob);
        m_hJob = nullptr;
        return;
    }

    // 4. 关联进程到 Job Object
    if (!AssignProcessToJobObject(m_hJob, hProcess)) {
        emit logUpdated("无法将进程加入 Job Object");
        CloseHandle(hProcess);
        CloseHandle(m_hJob);
        m_hJob = nullptr;
        return;
    }

    CloseHandle(hProcess); // 不再需要，Job Object 已持有进程
}

void LatexmkManager::closeJobObject()
{
    if (m_hJob) {
        // 关闭Job对象会终止所有归属其下的进程
        CloseHandle(m_hJob);
        m_hJob = nullptr;
    }
}

#endif
