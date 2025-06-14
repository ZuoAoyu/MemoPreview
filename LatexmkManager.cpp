#include "LatexmkManager.h"
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QCoreApplication>

LatexmkManager::LatexmkManager(QObject *parent)
{
    m_process = new QProcess(this);
    setupConnections();
}

LatexmkManager::~LatexmkManager()
{
    stop();
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
    if (m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished(3000);
    }
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

void LatexmkManager::handleProcessStarted()
{
    emitStatus("编译中");
}

void LatexmkManager::handleProcessError(QProcess::ProcessError error)
{
    Q_UNUSED(error)
    emitStatus("latexmk进程错误");
    emit logUpdated("latexmk 进程出错，准备重启...");

    // 只有不是用户主动 stop 时才自动重启
    if (!m_userStopping) {
        emit processCrashed();
        // 自动重启由主窗口处理
    }
}

void LatexmkManager::handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode)
    if (exitStatus == QProcess::CrashExit) {
        emitStatus("latexmk崩溃");
        emit logUpdated("latexmk崩溃，准备重启...");
        if (!m_userStopping)
            emit processCrashed();
    } else {
        emitStatus("latexmk已退出");
    }
}

void LatexmkManager::handleReadyReadStandardOutput()
{
    QByteArray out = m_process->readAllStandardOutput();
    m_logBuffer.append(out);
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
}

void LatexmkManager::setupConnections()
{
    connect(m_process, &QProcess::started, this, &LatexmkManager::handleProcessStarted);
    connect(m_process, QOverload<QProcess::ProcessError>::of(&QProcess::errorOccurred), this, &LatexmkManager::handleProcessError);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &LatexmkManager::handleProcessFinished);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &LatexmkManager::handleReadyReadStandardOutput);
    connect(m_process, &QProcess::readyReadStandardError, this, &LatexmkManager::handleReadyReadStandardError);
}
