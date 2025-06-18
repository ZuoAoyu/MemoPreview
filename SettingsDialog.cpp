#include "SettingsDialog.h"
#include "TemplateEditDialog.h"
#include "Config.h"

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
    QSettings settings{SOFTWARE_NAME, SOFTWARE_NAME};

    settings.setValue("lang", langSelector->currentData().toString());

    settings.setValue("latexmkPath", latexmkPathEdit->text());
    settings.setValue("latexmkArgs", latexmkArgsEdit->text());
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
    QSettings settings{SOFTWARE_NAME, SOFTWARE_NAME};

    QString curLang = settings.value("lang", QLocale::system().name()).toString();
    int idx = langSelector->findData(curLang);
    if (idx >= 0)
        langSelector->setCurrentIndex(idx);

    latexmkPathEdit->setText(settings.value("latexmkPath").toString());
    latexmkArgsEdit->setText(settings.value("latexmkArgs").toString());
    workspacePathEdit->setText(settings.value("workspacePath").toString());

    QStringList templates = settings.value("templates").toStringList();
    templateList->clear();
    templateContentMap.clear();

    for (const auto& title : templates) {
        QString content = settings.value("templateContent/" + title, "").toString();
        templateContentMap[title] = content;
        templateList->addItem(title);
    }
}

void SettingsDialog::browseLatexmk()
{
    QString fileName = QFileDialog::getOpenFileName(this, "选择latexmk路径");
    if (!fileName.isEmpty()) {
        latexmkPathEdit->setText(fileName);
    }
}

void SettingsDialog::browseWorkspace()
{
    QString dirName = QFileDialog::getExistingDirectory(this, "选择工作区目录");
    if (!dirName.isEmpty()) {
        workspacePathEdit->setText(dirName);
    }
}

void SettingsDialog::openWorkspace()
{
    QString path = workspacePathEdit->text();
    if (!path.isEmpty()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    } else {
        QMessageBox::warning(this, "警告", "工作区路径为空！");
    }
}

void SettingsDialog::setupUi()
{
    setWindowTitle("设置");

    auto* mainLayout = new QVBoxLayout{this};

    // 界面语言切换
    auto* langLayout = new QHBoxLayout;
    langLayout->addWidget(new QLabel(tr("界面语言:")));
    langSelector = new QComboBox(this);
    langSelector->addItem("简体中文", "zh_CN");
    langSelector->addItem("English", "en_US");
    langLayout->addWidget(langSelector);
    langLayout->addStretch();

    // latexmk 路径
    auto* latexmkLayout = new QHBoxLayout;
    latexmkLayout->addWidget(new QLabel{"latexmk 路径:"});
    latexmkPathEdit = new QLineEdit{this};
    latexmkLayout->addWidget(latexmkPathEdit);
    auto* browseLatexmkBtn = new QPushButton{"浏览", this};
    connect(browseLatexmkBtn, &QPushButton::clicked, this, &SettingsDialog::browseLatexmk);
    latexmkLayout->addWidget(browseLatexmkBtn);

    auto* argsLayout = new QHBoxLayout;
    argsLayout->addWidget(new QLabel{"latexmk 参数:"});
    latexmkArgsEdit = new QLineEdit{this};
    latexmkArgsEdit->setPlaceholderText("-pdf -pvc -interaction=nonstopmode -outdir=build");
    latexmkArgsEdit->setToolTip(
        "指定 latexmk 编译参数。\n"
        "常见示例：-pdf -pvc -interaction=nonstopmode -outdir=build\n"
        "说明：\n"
        "  -pvc  必填，表示持续监控文件并自动编译。\n"
        "  -pdf  用 pdflatex 编译。\n"
        "  -outdir=build  输出文件到 build 文件夹。\n"
        "如不确定建议保留默认。\n\n"
        "按照示例填写后，latexmk 运行时使用的完整命令：\n"
        "latexmk -pdf -pvc -interaction=nonstopmode -outdir=build main.tex\n"
        );
    argsLayout->addWidget(latexmkArgsEdit);

    // 工作区路径
    auto* workspaceLayout = new QHBoxLayout;
    workspaceLayout->addWidget(new QLabel{"工作区路径:"});
    workspacePathEdit = new QLineEdit{this};
    workspaceLayout->addWidget(workspacePathEdit);
    auto* browseWorkspaceBtn = new QPushButton{"浏览", this};
    connect(browseWorkspaceBtn, &QPushButton::clicked, this, &SettingsDialog::browseWorkspace);
    workspaceLayout->addWidget(browseWorkspaceBtn);

    auto* openWorkspaceBtn = new QPushButton{"一键打开", this};
    connect(openWorkspaceBtn, &QPushButton::clicked, this, &SettingsDialog::openWorkspace);
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

    mainLayout->addLayout(langLayout);
    mainLayout->addLayout(latexmkLayout);
    mainLayout->addLayout(argsLayout);
    mainLayout->addLayout(workspaceLayout);

    mainLayout->addWidget(templateLabel);
    mainLayout->addLayout(templateBtnLayout);
    mainLayout->addWidget(templateList);

    mainLayout->addLayout(btnLayout);
}
