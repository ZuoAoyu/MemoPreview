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

    mainLayout->addLayout(latexmkLayout);
}
