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
#include "WriteDebouncePolicy.h"

namespace {
constexpr int CONTENT_WRITE_DEBOUNCE_MS = 700;
constexpr int MIN_IMMEDIATE_WRITE_GAP_MS = 250;
constexpr int IE_REFRESH_ACTIVE_BASE_INTERVAL_MS = 250;
constexpr int IE_REFRESH_INACTIVE_BASE_INTERVAL_MS = 800;
constexpr int IE_REFRESH_MIN_IDLE_GAP_MS = 60;
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

    // 刷新定时器改为单次调度：下一轮只在本轮提取完成后安排，避免高频空转。
    ieRefreshTimer = new QTimer(this);
    ieRefreshTimer->setSingleShot(true);
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
        currentIeControls.clear();
        lastAllContentHash.clear();
        debounceTimer->stop();
        m_refreshPending = false;
        // 立即刷新
        refreshIeControls();
    });

    connect(latexTemplateSelector, &QComboBox::currentTextChanged, this, [this](const QString& title){
        currentTemplateTitle = title;
        m_templateService.saveLastSelectedTitle(title);

        // 可立即触发一次刷新内容写入 main.tex
        updateLatexSourceIfNeeded();
    });

    refreshSuperMemoWindowList();
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
    const quintptr previousHwnd = reinterpret_cast<quintptr>(currentSuperMemoHwnd);
    m_superMemoWindows = m_superMemoGateway.enumerateWindows();
    qInfo() << "[SM_WIN_SCAN] found windows:" << m_superMemoWindows.size();
    superWindowSelector->clear();
    int selectedIndex = -1;
    for (int i = 0; i < m_superMemoWindows.size(); ++i) {
        const auto& info = m_superMemoWindows[i];
        QString title = info.title.trimmed();
        if (title.isEmpty()) {
            title = QStringLiteral("未命名窗口");
        }
        if (info.isMinimized) {
            title += QStringLiteral(" [已最小化]");
        }
        QString label = QString("[%1] PID:%2")
            .arg(title).arg(info.processId);
        superWindowSelector->addItem(label);
        if (reinterpret_cast<quintptr>(info.hwnd) == previousHwnd) {
            selectedIndex = i;
        }
    }

    if (!m_superMemoWindows.isEmpty()) {
        if (selectedIndex < 0) {
            selectedIndex = 0;
        }
        superWindowSelector->setCurrentIndex(selectedIndex);
        currentSuperMemoHwnd = (HWND)m_superMemoWindows[selectedIndex].hwnd;
        currentIeControls.clear();
        lastAllContentHash.clear();
        m_refreshPending = false;
        updateSuperMemoStatus("已连接SuperMemo", true);
        refreshIeControls();
    } else {
        superWindowSelector->addItem("未发现SuperMemo窗口");
        currentSuperMemoHwnd = nullptr;
        updateSuperMemoStatus("未检测到SuperMemo窗口", false);
        ieTabWidget->clear();
        currentIeControls.clear();
        lastAllContentHash.clear();
        m_refreshPending = false;
        if (ieRefreshTimer) {
            ieRefreshTimer->stop();
        }
    }
}

void MainWindow::refreshIeControls()
{
    if (!currentSuperMemoHwnd || m_isShuttingDown) return;

    // 如果上次提取还在进行中，只记住“还需要再跑一次”。
    if (isExtracting) {
        m_refreshPending = true;
        return;
    }

    isExtracting = true;
    m_refreshPending = false;
    const HWND extractionTargetHwnd = currentSuperMemoHwnd;
    const auto previousControls = currentIeControls;
    QPointer<MainWindow> guardThis(this);

    // 开线程抓取，否则大窗口可能会卡
    m_extractionPool.start([guardThis, extractionTargetHwnd, previousControls]() {
        QElapsedTimer extractionClock;
        extractionClock.start();

        SuperMemoGateway gateway;
        const SuperMemoExtractionSnapshot snapshot = gateway.extractControls(extractionTargetHwnd, previousControls);
        auto allCtrls = snapshot.controls;
        const QString allHash = snapshot.contentHash;
        const int extractionDurationMs = static_cast<int>(extractionClock.elapsed());

        if (!guardThis) return;
        QMetaObject::invokeMethod(guardThis.data(), [guardThis, allCtrls = std::move(allCtrls), allHash, extractionTargetHwnd, extractionDurationMs]() mutable {
            if (!guardThis) return;
            auto* self = guardThis.data();
            self->isExtracting = false;

            if (self->m_isShuttingDown) return;
            const bool forceImmediate = self->m_refreshPending;
            self->m_refreshPending = false;

            if (self->currentSuperMemoHwnd != extractionTargetHwnd) {
                self->scheduleNextIeRefresh(0, true);
                return;
            }

            const bool snapshotChanged = (allHash != self->lastAllContentHash) || self->ieTabWidget->count() == 0;
            if (snapshotChanged) {
                self->lastAllContentHash = allHash;
                self->currentIeControls = allCtrls;

                self->ieTabWidget->clear();
                if (allCtrls.empty()) {
                    auto* info = new QTextEdit;
                    info->setReadOnly(true);
                    info->setPlainText("未检测到任何 Internet Explorer_Server 控件！");
                    self->ieTabWidget->addTab(info, "无控件");
                } else {
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
                }
            }

            self->scheduleNextIeRefresh(extractionDurationMs, forceImmediate);
        }, Qt::QueuedConnection);
    });
}

int MainWindow::currentRefreshBaseIntervalMs() const
{
    const bool isSuperMemoActive = m_superMemoGateway.isForegroundProcess(currentSuperMemoHwnd);
    return isSuperMemoActive
        ? IE_REFRESH_ACTIVE_BASE_INTERVAL_MS
        : IE_REFRESH_INACTIVE_BASE_INTERVAL_MS;
}

void MainWindow::scheduleNextIeRefresh(int extractionDurationMs, bool forceImmediate)
{
    if (!ieRefreshTimer || m_isShuttingDown || !currentSuperMemoHwnd) {
        return;
    }

    ieRefreshTimer->stop();
    if (forceImmediate) {
        ieRefreshTimer->start(0);
        return;
    }

    const int baseIntervalMs = currentRefreshBaseIntervalMs();
    const int delayMs = qMax(IE_REFRESH_MIN_IDLE_GAP_MS, baseIntervalMs - qMax(0, extractionDurationMs));
    ieRefreshTimer->start(delayMs);
}

void MainWindow::scheduleLatexUpdateOnContentChanged()
{
    // 第一次变化优先立即写，保证切卡后预览尽快更新；
    // 连续变化交给防抖定时器做一次尾部写入。
    const bool shouldWriteNow = WriteDebouncePolicy::shouldWriteImmediately(
        debounceTimer->isActive(),
        lastLatexWriteClock.isValid(),
        lastLatexWriteClock.isValid() ? lastLatexWriteClock.elapsed() : 0,
        MIN_IMMEDIATE_WRITE_GAP_MS);
    if (shouldWriteNow) {
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
