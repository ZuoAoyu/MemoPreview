#ifndef TEMPLATEEDITDIALOG_H
#define TEMPLATEEDITDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPlainTextEdit>

class TemplateEditDialog : public QDialog
{
    Q_OBJECT
public:
    explicit TemplateEditDialog(const QString& title = "", const QString& content = "", QWidget* parent = nullptr);

    QString getTemplateTitle() const;
    QString getTemplateContent() const;
private:
    QLineEdit* titleEdit;
    QPlainTextEdit* contentEdit;

    void setupUi(const QString& title, const QString& content);
};

#endif // TEMPLATEEDITDIALOG_H
