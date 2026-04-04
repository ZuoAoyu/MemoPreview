#ifndef LOGDIALOG_H
#define LOGDIALOG_H

#include <QDialog>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

class LogDialog : public QDialog
{
    Q_OBJECT
public:
    explicit LogDialog(QWidget* parent = nullptr);
    void appendLog(const QString& log);
    void clearLog();

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    QPlainTextEdit* logEdit;
    QPushButton* clearButton;
};

#endif // LOGDIALOG_H
