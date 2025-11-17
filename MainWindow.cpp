#include "MainWindow.h"
#include <QToolBar>
#include <QLabel>
#include <QSettings>
#include <QFileInfo>
#include <QStatusBar>
#include <QDebug>
#include <QShortcut>
#include <QtConcurrent>
#include <QDesktopServices>
#include <QMessageBox>
#include "SettingsDialog.h"
#include "SuperMemoWindowUtils.h"
#include "Config.h"
#include "SettingsUtils.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // 首次启动时的初始化设置
    SettingsUtils::ensureInitialSettings();

    createActions();
    createToolBars();
    createStatusBar();

    ieTabWidget = new QTabWidget(this);
    ieTabWidget->setTabPosition(QTabWidget::North);
    ieTabWidget->setDocumentMode(true);
    setCentralWidget(ieTabWidget);

    setWindowTitle(SOFTWARE_NAME);
    setMinimumSize(640, 320);
    // resize(480, 320);
    resize(sizeHint());

    m_latexmkMgr = new LatexmkManager(this);

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

    // 刷新定时器，防抖，每0.5s定时拉取
    // 定时轮询 SuperMemo 窗口内容，看看 IE 控件内容有没有变化
    ieRefreshTimer = new QTimer(this);
    ieRefreshTimer->setInterval(500); // 0.5秒拉取一次
    connect(ieRefreshTimer, &QTimer::timeout, this, &MainWindow::refreshIeControls);

    // 防止连续高频写入 main.tex 文件，只在内容稳定后再保存。
    debounceTimer = new QTimer(this);
    debounceTimer->setSingleShot(true);
    debounceTimer->setInterval(1000); // 1000ms防抖，内容连续变化时最后一次刷新
    connect(debounceTimer, &QTimer::timeout, this, &MainWindow::updateLatexSourceIfNeeded);

    connect(superWindowSelector, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx){
        if (idx < 0 || idx >= m_superMemoWindows.size()) return;
        currentSuperMemoHwnd = (HWND)m_superMemoWindows[idx].hwnd;
        ieTabWidget->clear();
        lastAllContentHash.clear();
        // 立即刷新
        refreshIeControls();
    });

    ieRefreshTimer->start();

    connect(latexTemplateSelector, &QComboBox::currentTextChanged, this, [this](const QString& title){
        currentTemplateTitle = title;

        // 记住用户所选项
        QSettings settings{SOFTWARE_NAME, SOFTWARE_NAME};
        settings.setValue("lastTemplateTitle", title);

        // 可立即触发一次刷新内容写入 main.tex
        updateLatexSourceIfNeeded();
    });

    loadAllTemplatesFromSettings();
}

MainWindow::~MainWindow() {
    if (ieRefreshTimer) ieRefreshTimer->stop();
    if (debounceTimer) debounceTimer->stop();

    delete m_latexmkMgr;
    m_latexmkMgr = nullptr;
}

void MainWindow::onStartLatexmk()
{
    QSettings settings{SOFTWARE_NAME, SOFTWARE_NAME};
    QString latexmkPath = settings.value("latexmkPath", "latexmk").toString();
    QString latexmkArgs = settings.value("latexmkArgs").toString();
    QString workspacePath = settings.value("workspacePath", "").toString();

    if (latexmkPath.isEmpty() || workspacePath.isEmpty()) {
        // 可弹窗提示
        statusBar()->showMessage("latexmk路径或工作区未设置", 3000);
        return;
    }

    // 强校验：是否含 -pvc 参数
    if (!latexmkArgs.trimmed().isEmpty() && !latexmkArgs.contains("-pvc")) {
        QMessageBox::warning(this, "警告", "latexmk 参数中必须包含 -pvc，否则无法持续编译！\n\n请加上 -pvc 参数。");
        return;
    }

    m_latexmkMgr->start(latexmkPath, workspacePath, latexmkArgs);
    startAction->setEnabled(false);
    stopAction->setEnabled(true);
}

void MainWindow::onStopLatexmk()
{
    m_latexmkMgr->stop();
    startAction->setEnabled(true);
    stopAction->setEnabled(false);
}

void MainWindow::openWorkspace()
{
    QSettings settings{SOFTWARE_NAME, SOFTWARE_NAME};
    QString workspacePath = settings.value("workspacePath").toString();
    if (!workspacePath.isEmpty()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(workspacePath));
    } else {
        QMessageBox::warning(this, "警告", "工作区路径未设置！");
    }
}

void MainWindow::createActions()
{
    refreshAction = new QAction{"刷新", this};
    refreshAction->setShortcut(tr("F5"));

    connect(refreshAction, &QAction::triggered, this, &MainWindow::refreshSuperMemoWindowList);

    showLogAction = new QAction{"日志", this};
    showConfigAction = new QAction{"设置", this};
    openWorkspaceAction = new QAction{"工作区", this};
    connect(openWorkspaceAction, &QAction::triggered, this, &MainWindow::openWorkspace);

    startAction = new QAction{"开始", this};
    stopAction = new QAction{"结束", this};
    stopAction->setEnabled(false);

    connect(showConfigAction, &QAction::triggered, this, [this](){
        SettingsDialog dlg(this);
        if (dlg.exec() == QDialog::Accepted) {
            loadAllTemplatesFromSettings(); // 刷新模板
            updateLatexSourceIfNeeded();
        }
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
        superWindowSelector->setMaximumWidth(200); // 最大宽度

        latexTemplateSelector = new QComboBox(settingToolBar);
        latexTemplateSelector->setToolTip("LaTeX 模板选择");
        latexTemplateSelector->setMaximumWidth(200); // 最大宽度
        // latexTemplateSelector->addItem("标准");
        // latexTemplateSelector->addItem("数学");

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

void MainWindow::createStatusBar()
{
    // 状态栏：编译/连接状态
    statusLabel = new QLabel("未连接SuperMemo", this);
    compileStatusLabel = new QLabel("未编译", this);
    statusBar()->addPermanentWidget(statusLabel);
    statusBar()->addPermanentWidget(compileStatusLabel);
}

void MainWindow::refreshSuperMemoWindowList()
{
    m_superMemoWindows = SuperMemoWindowUtils::enumerateAllSuperMemoWindows();
    superWindowSelector->clear();
    for (const auto& info : m_superMemoWindows) {
        QString label = QString("[%1] PID:%2")
        .arg(info.title).arg(info.processId);
        superWindowSelector->addItem(label);
    }

    if (!m_superMemoWindows.isEmpty()) {
        superWindowSelector->setCurrentIndex(0);
        currentSuperMemoHwnd = (HWND)m_superMemoWindows[0].hwnd;
        updateSuperMemoStatus("已连接SuperMemo", true);
        refreshIeControls();
    } else {
        superWindowSelector->addItem("未发现SuperMemo窗口");
        currentSuperMemoHwnd = nullptr;
        updateSuperMemoStatus("未检测到SuperMemo窗口", false);
        ieTabWidget->clear();
    }
}

void MainWindow::refreshIeControls()
{
    if (!currentSuperMemoHwnd) return;

    // 如果上次提取还在进行中，跳过本次
    if (isExtracting) return;

    // 检测SuperMemo窗口是否是前台窗口
    HWND foregroundWindow = GetForegroundWindow();
    bool isSuperMemoActive = (foregroundWindow == currentSuperMemoHwnd);

    // 如果SuperMemo不是活动窗口，降低轮询频率
    // 每4次轮询（2秒）才真正执行一次提取
    if (!isSuperMemoActive) {
        pollCounter++;
        if (pollCounter < 4) {
            return; // 跳过本次提取
        }
        pollCounter = 0; // 重置计数器
    } else {
        pollCounter = 0; // SuperMemo活跃时，每次都提取
    }

    isExtracting = true;

    // 开线程抓取，否则大窗口可能会卡
    QtConcurrent::run([this](){
        SuperMemoIeExtractor extractor(currentSuperMemoHwnd);
        auto allCtrls = extractor.extractAllIeControls();

        // 组装内容hash
        QStringList contentList;
        contentList.reserve(allCtrls.size());
        for (auto &ctrl : allCtrls) {
            contentList << ctrl.content;
        }

        QString allHash = contentList.join("");

        // 在返回前重置提取标志
        QMetaObject::invokeMethod(this, [this]() {
            isExtracting = false;
        });

        if (allHash == lastAllContentHash)
            return; // 内容未变，跳过

        lastAllContentHash = allHash;
        currentIeControls = allCtrls;

        // 更新UI（回到主线程）
        QMetaObject::invokeMethod(this, [this, allCtrls]() {
            ieTabWidget->clear();
            if (allCtrls.empty()) {
                auto* info = new QTextEdit;
                info->setReadOnly(true);
                info->setPlainText("未检测到任何 Internet Explorer_Server 控件！");
                ieTabWidget->addTab(info, "无控件");
                return;
            }
            // 以相反的顺序遍历控件，这样才和 SuperMemo 窗口中控件的顺序一致
            for (int i = allCtrls.size() - 1; i >= 0; --i) {
                auto &ctrl = allCtrls[i];
                auto* textEdit = new QTextEdit;
                textEdit->setReadOnly(true);
                textEdit->setPlainText(ctrl.content);
                QString tabLabel = QString("控件%1 %2").arg(allCtrls.size() - i).arg(ctrl.htmlTitle);
                ieTabWidget->addTab(textEdit, tabLabel);
            }

            debounceTimer->start();
        });
    });
}

void MainWindow::updateLatexSourceIfNeeded()
{
    // 将所有控件内容拼到 main.tex
    QSettings settings{SOFTWARE_NAME, SOFTWARE_NAME};
    QString workspacePath = settings.value("workspacePath").toString();
    if (workspacePath.isEmpty() || currentIeControls.empty() || currentTemplateTitle.isEmpty()) return;

    QString texFile = workspacePath + "/main.tex";

    QString memoContent;
    bool firstControl = true;
    // 多控件拼成多段
    for (int i = currentIeControls.size() - 1; i >= 0; --i) {
        if (!firstControl) memoContent += "\\newpage\n";
        const auto& ctrl = currentIeControls[i];
        memoContent += QString("%% === 控件%1 Title: %2 URL: %3\n")
                           .arg(i+1).arg(ctrl.htmlTitle).arg(ctrl.url);
        memoContent += ctrl.content + "\n\n";
        firstControl = false;
    }

    // 获取选中的模板内容
    QString templateText = templateContentMap.value(currentTemplateTitle);
    if (templateText.isEmpty()) templateText = "%CONTENT%";

    QString latexContent = templateText;
    latexContent.replace("%CONTENT%", memoContent);

    QFile file(texFile);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(latexContent.toUtf8());
        file.close();
    }
}

void MainWindow::updateSuperMemoStatus(const QString &status, bool good)
{
    // good=true为绿色，false为红色
    QString color = good ? "#19be6b" : "#e34f4f";
    statusLabel->setText(QString("<font color='%1'>%2</font>").arg(color).arg(status));
}

void MainWindow::loadAllTemplatesFromSettings()
{
    QSettings settings{SOFTWARE_NAME, SOFTWARE_NAME};
    QStringList templates = settings.value("templates").toStringList();
    QString lastTemplate = settings.value("lastTemplateTitle").toString();

    templateContentMap.clear();
    latexTemplateSelector->clear();
    for (const auto& title : templates) {
        QString content = settings.value("templateContent/" + title, "").toString();
        templateContentMap[title] = content;
        latexTemplateSelector->addItem(title);
    }

    if (templateContentMap.contains(lastTemplate)) {
        currentTemplateTitle = lastTemplate;
        latexTemplateSelector->setCurrentText(lastTemplate);
    } else if (!templateContentMap.isEmpty()) {
        // 自动切到第一个模板，并修正 lastTemplateTitle
        QString first = templateContentMap.firstKey();
        currentTemplateTitle = first;
        latexTemplateSelector->setCurrentText(first);
        settings.setValue("lastTemplateTitle", first);  // 自动修正
    } else {
        currentTemplateTitle.clear();
        settings.remove("lastTemplateTitle");
    }
}
