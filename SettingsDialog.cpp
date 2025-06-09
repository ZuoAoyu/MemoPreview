#include "SettingsDialog.h"
#include "TemplateEditDialog.h"

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

void SettingsDialog::addTemplate()
{
    TemplateEditDialog dlg("", "", this);
    dlg.setWindowTitle("新增模板");
    if (dlg.exec() == QDialog::Accepted) {
        QString title = dlg.getTemplateTitle();
        QString content = dlg.getTemplateContent();

        if (title.isEmpty()) {
            QMessageBox::warning(this, "提示", "模板标题不能为空！");
            return;
        }

        if (templateContentMap.contains(title)) {
            QMessageBox::warning(this, "提示", "模板标题已存在！");
            return;
        }

        templateContentMap[title] = content;
        templateList->addItem(title);
    }
}

void SettingsDialog::editTemplate()
{

}

void SettingsDialog::deleteTemplate()
{

}

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

    // 模板管理
    auto* templateLabel = new QLabel("模板管理:");
    templateList = new QListWidget{this};
    templateList->addItem("标准");
    templateList->addItem("数学");

    auto* templateBtnLayout = new QHBoxLayout;
    auto* addTemplateBtn = new QPushButton("新增模板", this);
    auto* editTemplateBtn = new QPushButton("编辑模板", this);
    auto* deleteTemplateBtn = new QPushButton("删除模板", this);
    templateBtnLayout->addWidget(addTemplateBtn);
    templateBtnLayout->addWidget(editTemplateBtn);
    templateBtnLayout->addWidget(deleteTemplateBtn);
    templateBtnLayout->addStretch();

    connect(addTemplateBtn, &QPushButton::clicked, this, &SettingsDialog::addTemplate);
    connect(editTemplateBtn, &QPushButton::clicked, this, &SettingsDialog::editTemplate);
    connect(deleteTemplateBtn, &QPushButton::clicked, this, &SettingsDialog::deleteTemplate);

    // 保存和取消按钮
    auto* btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    auto* saveBtn = new QPushButton("保存", this);
    auto* cancelBtn = new QPushButton("取消", this);
    btnLayout->addWidget(saveBtn);
    btnLayout->addWidget(cancelBtn);


    mainLayout->addLayout(latexmkLayout);
    mainLayout->addLayout(workspaceLayout);

    mainLayout->addWidget(templateLabel);
    mainLayout->addLayout(templateBtnLayout);
    mainLayout->addWidget(templateList);

    mainLayout->addLayout(btnLayout);
}
