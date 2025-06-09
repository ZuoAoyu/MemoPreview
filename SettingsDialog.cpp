#include "SettingsDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QSettings>
#include <QDesktopServices>
#include <QUrl>
#include <QMessageBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

SettingsDialog::SettingsDialog(QWidget* parent) : QDialog{parent} {
    setupUi();
}

SettingsDialog::~SettingsDialog() {}

void SettingsDialog::setupUi()
{
    setWindowTitle("设置");

    auto* mainLayout = new QVBoxLayout{this};

    // latexmk 路径
    auto* latexmkLayout = new QHBoxLayout;
    latexmkLayout->addWidget(new QLabel{"latexmk 路径:"});
    auto * latexmkPathEdit = new QLineEdit{this};
    latexmkLayout->addWidget(latexmkPathEdit);
    auto* browseLatexmkBtn = new QPushButton{"浏览", this};
    latexmkLayout->addWidget(browseLatexmkBtn);

    // 工作区路径
    auto* workspaceLayout = new QHBoxLayout;
    workspaceLayout->addWidget(new QLabel{"工作区路径:"});
    auto* workspacePathEdit = new QLineEdit{this};
    workspaceLayout->addWidget(workspacePathEdit);
    auto* browseWorkspaceBtn = new QPushButton{"浏览", this};
    workspaceLayout->addWidget(browseWorkspaceBtn);

    auto* openWorkspaceBtn = new QPushButton{"一键打开", this};
    workspaceLayout->addWidget(openWorkspaceBtn);

    mainLayout->addLayout(latexmkLayout);
    mainLayout->addLayout(workspaceLayout);
}
