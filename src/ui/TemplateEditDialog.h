#ifndef TEMPLATEEDITDIALOG_H
#define TEMPLATEEDITDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPlainTextEdit>

class TemplateEditDialog : public QDialog
{
    Q_OBJECT
public:
    explicit TemplateEditDialog(const QString& title = "", const QString& content = "", const QStringList& existingTitles = {},QWidget* parent = nullptr);

    QString getTemplateTitle() const;
    QString getTemplateContent() const;
private slots:
    void validateAndAccept();
private:
    QLineEdit* titleEdit;
    QPlainTextEdit* contentEdit;
    QStringList existingTitles; // 存储已有标题以便验证

    void setupUi(const QString& title, const QString& content);
};

#endif // TEMPLATEEDITDIALOG_H
