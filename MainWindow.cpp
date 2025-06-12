#include "MainWindow.h"
#include <QComboBox>
#include <QToolBar>
#include <QLabel>
#include <QPdfPageSelector>
#include <QSettings>
#include <QFileInfo>
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
}

MainWindow::~MainWindow() {}

void MainWindow::createActions()
{
    refreshAction = new QAction{"刷新", this};
    refreshAction->setShortcut(tr("F5"));

    showLogAction = new QAction{"日志", this};
    showConfigAction = new QAction{"设置", this};
    openWorkspaceAction = new QAction{"工作区", this};

    connect(showConfigAction, &QAction::triggered, this, [this](){
        SettingsDialog dlg(this);
        dlg.exec();
    });
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
