#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPdfDocument>
#include <QPdfView>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
private:
    void createActions();
    void createToolBars();

    QAction* refreshAction = nullptr;
    QAction* showLogAction = nullptr;
    QAction* showConfigAction = nullptr;
    QAction* openWorkspaceAction = nullptr;

    QAction* startAction = nullptr;
    QAction* stopAction = nullptr;

    void loadPdfDocument();

    QPdfDocument* m_pdfDocument = nullptr;
    QPdfView* m_pdfView = nullptr;
};
#endif // MAINWINDOW_H
