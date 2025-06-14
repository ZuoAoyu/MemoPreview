#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include "LatexmkManager.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
private slots:
    void onStartLatexmk();
    void onStopLatexmk();
private:
    void createActions();
    void createToolBars();

    QAction* refreshAction = nullptr;
    QAction* showLogAction = nullptr;
    QAction* showConfigAction = nullptr;
    QAction* openWorkspaceAction = nullptr;

    QAction* startAction = nullptr;
    QAction* stopAction = nullptr;

    LatexmkManager* m_latexmkMgr = nullptr;
    QLabel* statusLabel = nullptr;
    QLabel* compileStatusLabel = nullptr;
};
#endif // MAINWINDOW_H
