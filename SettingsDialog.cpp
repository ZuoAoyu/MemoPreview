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
    loadSettings();
}

SettingsDialog::~SettingsDialog() {}

/*
新增模板时，用户输入标题如果已存在，提示用户，不关闭对话框并保留输入内容。
*/
void SettingsDialog::addTemplate()
{
    QStringList titles = templateContentMap.keys(); // 当前已有的模板标题

    TemplateEditDialog dlg("", "", titles, this);
    dlg.setWindowTitle("新增模板");
    if (dlg.exec() == QDialog::Accepted) {
        QString title = dlg.getTemplateTitle();
        QString content = dlg.getTemplateContent();

        templateContentMap[title] = content;
        templateList->addItem(title);
    }
}

/*
编辑模板时，用户修改标题与原来相同，不提示；若修改成其他已存在的标题，则提示。
*/
void SettingsDialog::editTemplate()
{
    QListWidgetItem* currentItem = templateList->currentItem();
    if (!currentItem) {
        QMessageBox::warning(this, "提示", "请先选中要编辑的模板！");
        return;
    }

    QString originalTitle = currentItem->text();
    QStringList titles = templateContentMap.keys();
    titles.removeAll(originalTitle); // 排除自身标题，在标题重名验证时允许和原有标题同名

    TemplateEditDialog dlg{originalTitle, templateContentMap.value(originalTitle), titles, this};
    if (dlg.exec() == QDialog::Accepted) {
        QString newTitle = dlg.getTemplateTitle();
        QString newContent = dlg.getTemplateContent();

        templateContentMap.remove(originalTitle);
        templateContentMap[newTitle] = newContent;

        currentItem->setText(newTitle);
    }

}

void SettingsDialog::deleteTemplate()
{
    QListWidgetItem* currentItem = templateList->currentItem();
    if (!currentItem) {
        QMessageBox::warning(this, "提示", "请先选中要删除的模板！");
        return;
    }

    QString title = currentItem->text();
    templateContentMap.remove(title);
    delete currentItem;
}

/*
Qt 会自动选择合适的存储方式保存软件设置，比如在 Windows 下，会存到注册表里。

Windows：HKEY_CURRENT_USER\Software\MySoft\App标题
macOS：~/Library/Preferences/com.MySoft.App标题.plist
Linux：~/.config/MySoft/App标题.conf

QSettings 每次 setValue 都是立刻写入磁盘（或者操作系统持久化存储）的！
不需要调用 save() 之类的函数。只要调用 setValue() 就立即持久化了。
*/
void SettingsDialog::saveSettings()
{
    QSettings settings{"MySoft", "App标题"};
    settings.setValue("latexmkPath", latexmkPathEdit->text());
    settings.setValue("workspacePath", workspacePathEdit->text());

    QStringList templates;
    for (int i = 0; i < templateList->count(); ++i) {
        QString title = templateList->item(i)->text();
        templates << title;
        settings.setValue("templateContent/" + title, templateContentMap.value(title, ""));
    }
    settings.setValue("templates", templates);

    accept();
}

void SettingsDialog::loadSettings()
{
    QSettings settings{"MySoft", "App标题"};
    settings.setValue("latexmkPath", latexmkPathEdit->text());
    settings.setValue("workspacePath", workspacePathEdit->text());

    QStringList templates = settings.value("templates").toStringList();
    templateList->clear();

    for (const auto& title : templates) {
        QString content = settings.value("templateContent/" + title, "").toString();
        templateContentMap[title] = content;
        templateList->addItem(title);
    }
}

void SettingsDialog::setupUi()
{
    setWindowTitle("设置");

    auto* mainLayout = new QVBoxLayout{this};

    // latexmk 路径
    auto* latexmkLayout = new QHBoxLayout;
    latexmkLayout->addWidget(new QLabel{"latexmk 路径:"});
    latexmkPathEdit = new QLineEdit{this};
    latexmkLayout->addWidget(latexmkPathEdit);
    auto* browseLatexmkBtn = new QPushButton{"浏览", this};
    latexmkLayout->addWidget(browseLatexmkBtn);

    // 工作区路径
    auto* workspaceLayout = new QHBoxLayout;
    workspaceLayout->addWidget(new QLabel{"工作区路径:"});
    workspacePathEdit = new QLineEdit{this};
    workspaceLayout->addWidget(workspacePathEdit);
    auto* browseWorkspaceBtn = new QPushButton{"浏览", this};
    workspaceLayout->addWidget(browseWorkspaceBtn);

    auto* openWorkspaceBtn = new QPushButton{"一键打开", this};
    workspaceLayout->addWidget(openWorkspaceBtn);

    // 模板管理
    auto* templateLabel = new QLabel("模板管理:");
    templateList = new QListWidget{this};
    // templateList->addItem("标准");
    // templateList->addItem("数学");

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

    connect(saveBtn, &QPushButton::clicked, this, &SettingsDialog::saveSettings);
    connect(cancelBtn, &QPushButton::clicked, this, &SettingsDialog::reject);


    mainLayout->addLayout(latexmkLayout);
    mainLayout->addLayout(workspaceLayout);

    mainLayout->addWidget(templateLabel);
    mainLayout->addLayout(templateBtnLayout);
    mainLayout->addWidget(templateList);

    mainLayout->addLayout(btnLayout);
}
