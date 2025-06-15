#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QComboBox>
#include <QTabWidget>
#include <QTimer>
#include "LatexmkManager.h"
#include "LogDialog.h"
#include "SuperMemoWindowInfo.h"
#include "SuperMemoIeExtractor.h"

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
    void createStatusBar();

    void refreshSuperMemoWindowList();
    void refreshIeControls();
    void updateLatexSourceIfNeeded();

    void updateSuperMemoStatus(const QString& status, bool good = true);

    void loadAllTemplatesFromSettings();

    QAction* refreshAction = nullptr;
    QAction* showLogAction = nullptr;
    QAction* showConfigAction = nullptr;
    QAction* openWorkspaceAction = nullptr;

    QAction* startAction = nullptr;
    QAction* stopAction = nullptr;

    LatexmkManager* m_latexmkMgr = nullptr;
    QLabel* statusLabel = nullptr;
    QLabel* compileStatusLabel = nullptr;

    LogDialog* logDialog = nullptr;

    QComboBox* superWindowSelector = nullptr;
    QVector<SuperMemoWindowInfo> m_superMemoWindows;

    QTabWidget* ieTabWidget = nullptr;
    QTimer* ieRefreshTimer = nullptr;
    HWND currentSuperMemoHwnd = nullptr;
    std::vector<IeControlContent> currentIeControls;
    // 防抖定时器
    QTimer* debounceTimer = nullptr;
    QString lastAllContentHash;

    QComboBox* latexTemplateSelector = nullptr;
    QMap<QString, QString> templateContentMap; // 模板标题->内容
    QString currentTemplateTitle;
};
#endif // MAINWINDOW_H
