#include "MainWindow.h"
#include <QComboBox>
#include <QToolBar>
#include <QLabel>
#include <QPdfPageSelector>
#include <QSettings>
#include <QFileInfo>
#include <QStatusBar>
#include <QDebug>
#include "SettingsDialog.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    createActions();
    createToolBars();

    setWindowTitle("在CMake中统一设置");
    setMinimumSize(160, 160);
    resize(480, 320);

    // PDF 视图页
    m_pdfDocument = new QPdfDocument(this);
    m_pdfView = new QPdfView(this);
    m_pdfView->setDocument(m_pdfDocument);

    // 设置主窗口中心部件
    setCentralWidget(m_pdfView);

    // 加载 PDF
    loadPdfDocument();

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
    connect(m_latexmkMgr, &LatexmkManager::pdfUpdated, this, &MainWindow::loadPdfDocument);
    connect(m_latexmkMgr, &LatexmkManager::processCrashed, this, [this](){
        m_latexmkMgr->restart();
    });
}

MainWindow::~MainWindow() {}

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

    // 添加换行。在一个新行上显示下面的工具栏
    addToolBarBreak();

    {
        QToolBar *pdfToolBar = new QToolBar;

        QPdfPageSelector* pageSelector = new QPdfPageSelector(pdfToolBar);
        pageSelector->setToolTip("翻页");
        pdfToolBar->addWidget(pageSelector);

        pdfToolBar->addSeparator(); // 插入分隔符，增加间距

        QComboBox* zoomSelector = new QComboBox(pdfToolBar);
        zoomSelector->setToolTip("缩放");
        zoomSelector->addItem(tr("Fit Width"));
        zoomSelector->addItem(tr("Fit Page"));
        zoomSelector->addItem(tr("12%"));
        zoomSelector->addItem(tr("25%"));
        zoomSelector->addItem(tr("33%"));
        zoomSelector->addItem(tr("50%"));
        zoomSelector->addItem(tr("66%"));
        zoomSelector->addItem(tr("75%"));
        zoomSelector->addItem(tr("100%"));
        zoomSelector->addItem(tr("125%"));
        zoomSelector->addItem(tr("150%"));
        zoomSelector->addItem(tr("200%"));
        zoomSelector->addItem(tr("400%"));

        pdfToolBar->addWidget(zoomSelector);

        auto* actionZoomIn = pdfToolBar->addAction("放大", QKeySequence::ZoomIn);
        auto* actionZoomOut = pdfToolBar->addAction("缩小", QKeySequence::ZoomOut);

        addToolBar(pdfToolBar);
    }
}

void MainWindow::loadPdfDocument()
{
    QSettings settings{"MySoft", "App标题"};
    QString workspacePath = settings.value("workspacePath", "").toString();
    if (workspacePath.isEmpty()) {
        return;
    }
    QString pdfPath = workspacePath + "/main.pdf";
    if (!QFileInfo::exists(pdfPath)) {
        return;
    }
    m_pdfDocument->load(pdfPath);
}
