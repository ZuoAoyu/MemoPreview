#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QStringList>
#include <QListWidget>
#include <QComboBox>

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

    void saveSettings();
    void loadSettings();

    void browseLatexmk();
    void browseWorkspace();
    void openWorkspace();
private:
    void setupUi();

private:
    // 存储模板内容，key为模板标题
    QMap<QString, QString> templateContentMap;

    QListWidget* templateList;

    QLineEdit* latexmkPathEdit;
    QLineEdit* latexmkArgsEdit;
    QLineEdit* workspacePathEdit;

    QComboBox* langSelector;
};

#endif // SETTINGSDIALOG_H
