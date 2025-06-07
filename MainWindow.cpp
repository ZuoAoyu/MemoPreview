#include "MainWindow.h"
#include <QComboBox>
#include <QToolBar>
#include <QLabel>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    createActions();
    createToolBars();

    setWindowTitle("在CMake中统一设置");
    setMinimumSize(160, 160);
    resize(480, 320);
}

MainWindow::~MainWindow() {}

void MainWindow::createActions()
{
    refreshAction = new QAction{"刷新", this};
    refreshAction->setShortcut(tr("F5"));

    showLogAction = new QAction{"日志", this};
    showConfigAction = new QAction{"设置", this};
    openWorkspaceAction = new QAction{"工作区", this};
}

void MainWindow::createToolBars()
{
    QToolBar *settingToolBar = new QToolBar;

    QComboBox* superWindowSelector = new QComboBox(settingToolBar);
    superWindowSelector->setToolTip("SuperMemo 窗口选择");
    superWindowSelector->addItem("SuperMemo 1");
    superWindowSelector->addItem("SuperMemo 2");
    superWindowSelector->addItem("SuperMemo 3");
    superWindowSelector->addItem("SuperMemo 4");
    superWindowSelector->addItem("SuperMemo 5");
    superWindowSelector->addItem("SuperMemo 6");

    QComboBox* latexTemplateSelector = new QComboBox(settingToolBar);
    latexTemplateSelector->setToolTip("LaTeX 模板选择");
    latexTemplateSelector->addItem("标准");
    latexTemplateSelector->addItem("数学");

    settingToolBar->addWidget(superWindowSelector);
    settingToolBar->addWidget(latexTemplateSelector);


    QToolBar *operateToolBar = new QToolBar;
    operateToolBar->addAction(refreshAction);
    operateToolBar->addAction(showLogAction);
    operateToolBar->addAction(showConfigAction);
    operateToolBar->addAction(openWorkspaceAction);

    addToolBar(settingToolBar);
    addToolBar(operateToolBar);
}
