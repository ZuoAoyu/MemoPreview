#include "TemplateEditDialog.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QMessageBox>

TemplateEditDialog::TemplateEditDialog(const QString &title, const QString &content, const QStringList &existingTitles, QWidget *parent)
    :QDialog{parent}, existingTitles{existingTitles}
{
    setupUi(title, content);
}

QString TemplateEditDialog::getTemplateTitle() const
{
    return titleEdit->text();
}

QString TemplateEditDialog::getTemplateContent() const
{
    return contentEdit->toPlainText();
}

void TemplateEditDialog::validateAndAccept()
{
    QString currentTitle = titleEdit->text().trimmed();

    if (currentTitle.isEmpty()) {
        QMessageBox::warning(this, "提示", "模板标题不能为空！");
        titleEdit->setFocus();
        return;
    }

    // 检查模板标题是否已存在
    if (existingTitles.contains(currentTitle)) {
        QMessageBox::warning(this, "提示", "模板标题已存在！");
        titleEdit->setFocus();
        return;
    }

    accept();
}

void TemplateEditDialog::setupUi(const QString &title, const QString &content)
{
    setWindowTitle("编辑模板");
    setMinimumSize(400,300);

    auto* mainLayout = new QVBoxLayout(this);

    auto* titleLayout = new QHBoxLayout;
    titleLayout->addWidget(new QLabel("模板标题:", this));
    titleEdit = new QLineEdit(title, this);
    titleLayout->addWidget(titleEdit);

    contentEdit = new QPlainTextEdit(content, this);

    auto* btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    auto* okBtn = new QPushButton("确定", this);
    auto* cancelBtn = new QPushButton("取消", this);
    btnLayout->addWidget(okBtn);
    btnLayout->addWidget(cancelBtn);

    connect(okBtn, &QPushButton::clicked, this, &TemplateEditDialog::validateAndAccept);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    mainLayout->addLayout(titleLayout);
    mainLayout->addWidget(new QLabel("模板内容:", this));
    mainLayout->addWidget(contentEdit);
    mainLayout->addLayout(btnLayout);
}
