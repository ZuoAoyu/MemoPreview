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
private:
    void setupUi();
};

#endif // SETTINGSDIALOG_H
