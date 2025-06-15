#include "MainWindow.h"
#include <QToolBar>
#include <QLabel>
#include <QSettings>
#include <QFileInfo>
#include <QStatusBar>
#include <QDebug>
#include <QShortcut>
#include "SettingsDialog.h"
#include "SuperMemoWindowUtils.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    createActions();
    createToolBars();

    setWindowTitle("在CMake中统一设置");
    setMinimumSize(160, 160);
    resize(480, 320);

    m_latexmkMgr = new LatexmkManager(this);

    // 状态栏：编译/连接状态
    statusLabel = new QLabel("未连接SuperMemo", this);
    compileStatusLabel = new QLabel("未编译", this);
    statusBar()->addPermanentWidget(statusLabel);
    statusBar()->addPermanentWidget(compileStatusLabel);

    // 连接latexmk manager信号
    connect(m_latexmkMgr, &LatexmkManager::compileStatusChanged, compileStatusLabel, &QLabel::setText);
    connect(m_latexmkMgr, &LatexmkManager::logUpdated, this, [this](const QString& log){
        // 日志窗口逻辑，可弹窗或追加到文本区域
    });
    connect(m_latexmkMgr, &LatexmkManager::processCrashed, this, [this](){
        m_latexmkMgr->restart();
    });
    connect(m_latexmkMgr, &LatexmkManager::processStopped, this, [this](){
        startAction->setEnabled(true);
        stopAction->setEnabled(false);
    });

    // 日志弹窗
    logDialog = new LogDialog(this);

    // 日志信号连接
    connect(m_latexmkMgr, &LatexmkManager::logUpdated, this, [this](const QString& log){
        logDialog->appendLog(log);
    });

    // 日志按钮事件
    connect(showLogAction, &QAction::triggered, this, [this](){
        logDialog->show();
        logDialog->raise();
        logDialog->activateWindow();
    });

    // 快捷键弹出日志窗口
    QShortcut* logShortcut = new QShortcut(QKeySequence(tr("Ctrl+L")), this);
    connect(logShortcut, &QShortcut::activated, this, [this](){
        logDialog->show();
        logDialog->raise();
        logDialog->activateWindow();
    });

    refreshSuperMemoWindowList();
}

MainWindow::~MainWindow() {
    delete m_latexmkMgr;
    m_latexmkMgr = nullptr;
}

void MainWindow::onStartLatexmk()
{
    QSettings settings{"MySoft", "App标题"};
    QString latexmkPath = settings.value("latexmkPath", "latexmk").toString();
    QString workspacePath = settings.value("workspacePath", "").toString();

    if (latexmkPath.isEmpty() || workspacePath.isEmpty()) {
        // 可弹窗提示
        statusBar()->showMessage("latexmk路径或工作区未设置", 3000);
        return;
    }

    m_latexmkMgr->start(latexmkPath, workspacePath);
    startAction->setEnabled(false);
    stopAction->setEnabled(true);
}

void MainWindow::onStopLatexmk()
{
    m_latexmkMgr->stop();
    startAction->setEnabled(true);
    stopAction->setEnabled(false);
}

void MainWindow::createActions()
{
    refreshAction = new QAction{"刷新", this};
    refreshAction->setShortcut(tr("F5"));

    connect(refreshAction, &QAction::triggered, this, &MainWindow::refreshSuperMemoWindowList);

    showLogAction = new QAction{"日志", this};
    showConfigAction = new QAction{"设置", this};
    openWorkspaceAction = new QAction{"工作区", this};

    startAction = new QAction{"开始", this};
    stopAction = new QAction{"结束", this};
    stopAction->setEnabled(false);

    connect(showConfigAction, &QAction::triggered, this, [this](){
        SettingsDialog dlg(this);
        dlg.exec();
    });

    // 按钮事件
    connect(startAction, &QAction::triggered, this, &MainWindow::onStartLatexmk);
    connect(stopAction, &QAction::triggered, this, &MainWindow::onStopLatexmk);
}

void MainWindow::createToolBars()
{
    {
        QToolBar *settingToolBar = new QToolBar;

        superWindowSelector = new QComboBox(settingToolBar);
        superWindowSelector->setToolTip("SuperMemo 窗口选择");
        // superWindowSelector->addItem("SuperMemo 1");
        // superWindowSelector->addItem("SuperMemo 2");

        QComboBox* latexTemplateSelector = new QComboBox(settingToolBar);
        latexTemplateSelector->setToolTip("LaTeX 模板选择");
        latexTemplateSelector->addItem("标准");
        latexTemplateSelector->addItem("数学");

        settingToolBar->addWidget(superWindowSelector);
        settingToolBar->addSeparator(); // 插入分隔符，增加间距
        settingToolBar->addWidget(latexTemplateSelector);

        addToolBar(settingToolBar);
    }

    {
        QToolBar *operateToolBar = new QToolBar;
        operateToolBar->addAction(startAction);
        operateToolBar->addAction(stopAction);
        operateToolBar->addAction(refreshAction);
        operateToolBar->addAction(showLogAction);
        operateToolBar->addAction(showConfigAction);
        operateToolBar->addAction(openWorkspaceAction);

        addToolBar(operateToolBar);
    }
}

void MainWindow::refreshSuperMemoWindowList()
{
    m_superMemoWindows = SuperMemoWindowUtils::enumerateAllSuperMemoWindows();
    superWindowSelector->clear();
    for (const auto& info : m_superMemoWindows) {
        QString label = QString("PID:%1 [%2] %3")
        .arg(info.processId)
            .arg(info.title)
            .arg(info.processExe.isEmpty() ? QString() : info.processExe);
        superWindowSelector->addItem(label);
    }

    if (!m_superMemoWindows.isEmpty()) {
        superWindowSelector->setCurrentIndex(0);
    } else {
        superWindowSelector->addItem("未发现SuperMemo窗口");
    }
}
