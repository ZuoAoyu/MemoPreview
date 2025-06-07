#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

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
};
#endif // MAINWINDOW_H
