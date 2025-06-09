#include "TemplateEditDialog.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>

TemplateEditDialog::TemplateEditDialog(const QString &title, const QString &content, QWidget *parent)
    :QDialog{parent}
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

    mainLayout->addLayout(titleLayout);
    mainLayout->addWidget(new QLabel("模板内容:", this));
    mainLayout->addWidget(contentEdit);
    mainLayout->addLayout(btnLayout);
}
