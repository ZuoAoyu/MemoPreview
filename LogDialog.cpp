#include "LogDialog.h"

LogDialog::LogDialog(QWidget *parent)
    :QDialog{parent}
{
    setWindowTitle("编译日志");
    setMinimumSize(640, 320);
    logEdit = new QPlainTextEdit(this);
    logEdit->setReadOnly(true);
    logEdit->setWordWrapMode(QTextOption::NoWrap);

    clearButton = new QPushButton("清空", this);
    connect(clearButton, &QPushButton::clicked, this, &LogDialog::clearLog);

    auto btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    btnLayout->addWidget(clearButton);

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(logEdit);
    mainLayout->addLayout(btnLayout);

    setLayout(mainLayout);
}

void LogDialog::appendLog(const QString &log)
{
    logEdit->appendPlainText(log);
    // 滚动到最后
    QTextCursor cursor = logEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    logEdit->setTextCursor(cursor);

    // 裁剪日志（最多1000行，超出删除前面部分）
    if (logEdit->blockCount() > 1000) {
        QTextCursor trimCursor(logEdit->document());
        trimCursor.movePosition(QTextCursor::Start);
        for (int i = 0; i < 200; ++i) { // 删除200行
            trimCursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
        }
        trimCursor.removeSelectedText();
    }
}

void LogDialog::clearLog()
{
    logEdit->clear();
}

void LogDialog::closeEvent(QCloseEvent *event)
{
    // 只隐藏，不真正销毁窗口
    event->ignore();
    this->hide();
}
