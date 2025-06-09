#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QStringList>
#include <QListWidget>

class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget* parent = nullptr);
    ~SettingsDialog();
private slots:
    void addTemplate();
    void editTemplate();
    void deleteTemplate();
private:
    void setupUi();
private:
    // 存储模板内容，key为模板标题
    QMap<QString, QString> templateContentMap;

    QListWidget* templateList;
};

#endif // SETTINGSDIALOG_H
