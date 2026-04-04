#ifndef LATEXCOMPILESERVICE_H
#define LATEXCOMPILESERVICE_H

#include <QObject>
#include <QString>
#include <QTimer>
#include "LatexmkManager.h"
#include "RetryBackoffPolicy.h"

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
    RetryBackoffPolicy m_backoffPolicy{1000, 10000};
    State m_state = State::Stopped;
    bool m_autoRestartEnabled = false;
    bool m_restartScheduled = false;

    QString m_lastLatexmkPath;
    QString m_lastWorkspaceDir;
    QString m_lastLatexmkArgs;
};

#endif // LATEXCOMPILESERVICE_H
