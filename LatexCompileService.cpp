#include "LatexCompileService.h"

#include <QDebug>

LatexCompileService::LatexCompileService(QObject* parent)
    : QObject(parent), m_manager(this)
{
    connect(&m_manager, &LatexmkManager::compileStatusChanged, this, &LatexCompileService::compileStatusChanged);
    connect(&m_manager, &LatexmkManager::logUpdated, this, &LatexCompileService::logUpdated);

    connect(&m_manager, &LatexmkManager::processStopped, this, [this]() {
        if (!m_restartScheduled) {
            setState(State::Stopped);
            emit runningStateChanged(false);
        }
    });

    connect(&m_manager, &LatexmkManager::processCrashed, this, [this]() {
        if (!m_autoRestartEnabled) {
            setState(State::Crashed);
            emit runningStateChanged(false);
            return;
        }
        scheduleRestart();
    });
}

bool LatexCompileService::start(const QString& latexmkPath, const QString& workspaceDir, const QString& latexmkArgs, QString* error)
{
    if (latexmkPath.isEmpty() || workspaceDir.isEmpty()) {
        if (error) {
            *error = "latexmk path or workspace is empty";
        }
        return false;
    }

    m_lastLatexmkPath = latexmkPath;
    m_lastWorkspaceDir = workspaceDir;
    m_lastLatexmkArgs = latexmkArgs;
    m_autoRestartEnabled = true;
    m_restartScheduled = false;

    m_manager.start(latexmkPath, workspaceDir, latexmkArgs);
    setState(State::Running);
    emit runningStateChanged(true);
    return true;
}

void LatexCompileService::stop()
{
    m_autoRestartEnabled = false;
    m_restartScheduled = false;
    m_manager.stop();
    setState(State::Stopped);
    emit runningStateChanged(false);
}

bool LatexCompileService::isRunning() const
{
    return m_state == State::Running;
}

LatexCompileService::State LatexCompileService::state() const
{
    return m_state;
}

void LatexCompileService::setState(State state)
{
    if (m_state == state) {
        return;
    }
    m_state = state;
    emit stateChanged(m_state);
}

void LatexCompileService::scheduleRestart()
{
    if (m_restartScheduled) {
        return;
    }
    m_restartScheduled = true;
    setState(State::Crashed);
    emit runningStateChanged(false);
    emit logUpdated(QString("[LATEX_AUTO_RESTART] restarting in %1ms").arg(m_restartDelayMs));

    QTimer::singleShot(m_restartDelayMs, this, [this]() {
        if (!m_autoRestartEnabled) {
            m_restartScheduled = false;
            return;
        }

        m_restartScheduled = false;
        QString error;
        if (!start(m_lastLatexmkPath, m_lastWorkspaceDir, m_lastLatexmkArgs, &error)) {
            qWarning() << "[LATEX_RESTART_FAILED]" << error;
        }
    });
}
