#include "MainWindow.h"
#include <QToolBar>
#include <QLabel>
#include <QSettings>
#include <QFileInfo>
#include <QStatusBar>
#include <QDebug>
#include <QShortcut>
#include <QDesktopServices>
#include <QMessageBox>
#include <QThreadPool>
#include <QPointer>
#include <QTextEdit>
#include "SettingsDialog.h"
#include "Config.h"
#include "SettingsUtils.h"

namespace {
constexpr int CONTENT_WRITE_DEBOUNCE_MS = 700;
constexpr int MIN_IMMEDIATE_WRITE_GAP_MS = 250;
constexpr int IE_REFRESH_ACTIVE_INTERVAL_MS = 250;
constexpr int IE_REFRESH_INACTIVE_INTERVAL_MS = 800;
}

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

    m_compileService = new LatexCompileService(this);
    m_extractionPool.setMaxThreadCount(1);
    m_extractionPool.setExpiryTimeout(-1);

    // 连接编译服务信号
    connect(m_compileService, &LatexCompileService::compileStatusChanged, compileStatusLabel, &QLabel::setText);
    connect(m_compileService, &LatexCompileService::logUpdated, this, [this](const QString& log){
        // 日志窗口逻辑，可弹窗或追加到文本区域
    });
    connect(m_compileService, &LatexCompileService::runningStateChanged, this, [this](bool running){
        startAction->setEnabled(!running);
        stopAction->setEnabled(running);
    });

    // 日志弹窗
    logDialog = new LogDialog(this);

    // 日志信号连接
    connect(m_compileService, &LatexCompileService::logUpdated, this, [this](const QString& log){
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
    ieRefreshTimer->setInterval(IE_REFRESH_ACTIVE_INTERVAL_MS);
    connect(ieRefreshTimer, &QTimer::timeout, this, &MainWindow::refreshIeControls);

    // 防止连续高频写入 main.tex 文件，只在内容稳定后再保存。
    debounceTimer = new QTimer(this);
    debounceTimer->setSingleShot(true);
    debounceTimer->setInterval(CONTENT_WRITE_DEBOUNCE_MS);
    connect(debounceTimer, &QTimer::timeout, this, &MainWindow::updateLatexSourceIfNeeded);

    connect(superWindowSelector, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx){
        if (idx < 0 || idx >= m_superMemoWindows.size()) return;
        currentSuperMemoHwnd = (HWND)m_superMemoWindows[idx].hwnd;
        ieTabWidget->clear();
        lastAllContentHash.clear();
        debounceTimer->stop();
        // 立即刷新
        refreshIeControls();
    });

    ieRefreshTimer->start();

    connect(latexTemplateSelector, &QComboBox::currentTextChanged, this, [this](const QString& title){
        currentTemplateTitle = title;
        m_templateService.saveLastSelectedTitle(title);

        // 可立即触发一次刷新内容写入 main.tex
        updateLatexSourceIfNeeded();
    });

    loadAllTemplatesFromSettings();
}

MainWindow::~MainWindow() {
    m_isShuttingDown = true;
    if (ieRefreshTimer) ieRefreshTimer->stop();
    if (debounceTimer) debounceTimer->stop();
    m_extractionPool.waitForDone();

    delete m_compileService;
    m_compileService = nullptr;
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

    QString error;
    if (!m_compileService->start(latexmkPath, workspacePath, latexmkArgs, &error)) {
        statusBar()->showMessage("启动编译失败: " + error, 3000);
    }
}

void MainWindow::onStopLatexmk()
{
    m_compileService->stop();
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
    m_superMemoWindows = m_superMemoGateway.enumerateWindows();
    qInfo() << "[SM_WIN_SCAN] found windows:" << m_superMemoWindows.size();
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
    if (!currentSuperMemoHwnd || m_isShuttingDown) return;

    // 如果上次提取还在进行中，跳过本次
    if (isExtracting) return;

    // 检测SuperMemo所属进程是否位于前台（比窗口句柄相等更稳）
    const bool isSuperMemoActive = m_superMemoGateway.isForegroundProcess(currentSuperMemoHwnd);

    // 前台时高频更新，后台时仍保持亚秒级刷新，避免“几秒后才更新”
    const int targetInterval = isSuperMemoActive
        ? IE_REFRESH_ACTIVE_INTERVAL_MS
        : IE_REFRESH_INACTIVE_INTERVAL_MS;
    if (ieRefreshTimer && ieRefreshTimer->interval() != targetInterval) {
        ieRefreshTimer->setInterval(targetInterval);
    }

    isExtracting = true;
    const HWND extractionTargetHwnd = currentSuperMemoHwnd;
    QPointer<MainWindow> guardThis(this);

    // 开线程抓取，否则大窗口可能会卡
    m_extractionPool.start([guardThis, extractionTargetHwnd]() {
        if (!guardThis) return;
        const SuperMemoExtractionSnapshot snapshot = guardThis->m_superMemoGateway.extractControls(extractionTargetHwnd);
        auto allCtrls = snapshot.controls;
        const QString allHash = snapshot.contentHash;

        if (!guardThis) return;
        QMetaObject::invokeMethod(guardThis.data(), [guardThis, allCtrls = std::move(allCtrls), allHash, extractionTargetHwnd]() mutable {
            if (!guardThis) return;
            auto* self = guardThis.data();
            self->isExtracting = false;

            if (self->m_isShuttingDown) return;
            if (self->currentSuperMemoHwnd != extractionTargetHwnd) return;
            if (allHash == self->lastAllContentHash) return;

            self->lastAllContentHash = allHash;
            self->currentIeControls = allCtrls;

            self->ieTabWidget->clear();
            if (allCtrls.empty()) {
                auto* info = new QTextEdit;
                info->setReadOnly(true);
                info->setPlainText("未检测到任何 Internet Explorer_Server 控件！");
                self->ieTabWidget->addTab(info, "无控件");
                return;
            }
            // 以相反的顺序遍历控件，这样才和 SuperMemo 窗口中控件的顺序一致
            for (int i = allCtrls.size() - 1; i >= 0; --i) {
                auto &ctrl = allCtrls[i];
                auto* textEdit = new QTextEdit;
                textEdit->setReadOnly(true);
                textEdit->setPlainText(ctrl.content);
                QString tabLabel = QString("控件%1 %2").arg(allCtrls.size() - i).arg(ctrl.htmlTitle);
                self->ieTabWidget->addTab(textEdit, tabLabel);
            }

            self->scheduleLatexUpdateOnContentChanged();
        }, Qt::QueuedConnection);
    });
}

void MainWindow::scheduleLatexUpdateOnContentChanged()
{
    const bool firstChangeInBurst = !debounceTimer->isActive();
    const bool canWriteImmediately = !lastLatexWriteClock.isValid()
        || lastLatexWriteClock.elapsed() >= MIN_IMMEDIATE_WRITE_GAP_MS;

    // 第一次变化优先立即写，保证切卡后预览尽快更新；
    // 连续变化交给防抖定时器做一次尾部写入。
    if (firstChangeInBurst && canWriteImmediately) {
        updateLatexSourceIfNeeded();
    }

    debounceTimer->start();
}

void MainWindow::updateLatexSourceIfNeeded()
{
    // 将所有控件内容拼到 main.tex
    QSettings settings{SOFTWARE_NAME, SOFTWARE_NAME};
    QString workspacePath = settings.value("workspacePath").toString();
    if (workspacePath.isEmpty() || currentIeControls.empty() || currentTemplateTitle.isEmpty()) return;
    const QString templateText = templateContentMap.value(currentTemplateTitle);
    const QString latexContent = m_previewSyncService.buildLatexContent(templateText, currentIeControls);

    if (latexContent == lastWrittenLatexContent) {
        return;
    }

    QString error;
    if (m_previewSyncService.writeMainTex(workspacePath, latexContent, &error)) {
        lastWrittenLatexContent = latexContent;
        lastLatexWriteClock.start();
    } else {
        qWarning() << "[WRITE_MAIN_TEX_FAILED] workspace=" << workspacePath << "error=" << error;
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
    const TemplateSnapshot snapshot = m_templateService.loadFromSettings();
    templateContentMap.clear();
    latexTemplateSelector->clear();
    templateContentMap = snapshot.contentMap;

    for (const auto& title : snapshot.titles) {
        latexTemplateSelector->addItem(title);
    }

    if (!snapshot.currentTitle.isEmpty() && templateContentMap.contains(snapshot.currentTitle)) {
        currentTemplateTitle = snapshot.currentTitle;
        latexTemplateSelector->setCurrentText(snapshot.currentTitle);
    } else {
        currentTemplateTitle.clear();
    }
}
