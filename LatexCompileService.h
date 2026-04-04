#ifndef LATEXCOMPILESERVICE_H
#define LATEXCOMPILESERVICE_H

#include <QObject>
#include <QString>
#include <QTimer>
#include "LatexmkManager.h"

class LatexCompileService : public QObject
{
    Q_OBJECT
public:
    enum class State {
        Stopped,
        Running,
        Crashed
    };

    explicit LatexCompileService(QObject* parent = nullptr);

    bool start(const QString& latexmkPath, const QString& workspaceDir, const QString& latexmkArgs, QString* error = nullptr);
    void stop();

    bool isRunning() const;
    State state() const;

signals:
    void compileStatusChanged(const QString& status);
    void logUpdated(const QString& log);
    void runningStateChanged(bool running);
    void stateChanged(LatexCompileService::State state);

private:
    void setState(State state);
    void scheduleRestart();

private:
    LatexmkManager m_manager;
    State m_state = State::Stopped;
    bool m_autoRestartEnabled = false;
    bool m_restartScheduled = false;
    int m_restartDelayMs = 1000;

    QString m_lastLatexmkPath;
    QString m_lastWorkspaceDir;
    QString m_lastLatexmkArgs;
};

#endif // LATEXCOMPILESERVICE_H
