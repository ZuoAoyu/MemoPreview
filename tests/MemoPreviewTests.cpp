#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QString>
#include <iostream>

#include "PreviewSyncService.h"
#include "WriteDebouncePolicy.h"
#include "RetryBackoffPolicy.h"

namespace {
int g_failures = 0;

void expectTrue(bool condition, const QString& message)
{
    if (!condition) {
        std::cerr << "[FAIL] " << message.toStdString() << std::endl;
        ++g_failures;
    }
}

void testPreviewSyncService()
{
    PreviewSyncService service;

    IeControlContent first;
    first.htmlTitle = "Title-A";
    first.url = "http://a";
    first.content = "Content-A";

    IeControlContent second;
    second.htmlTitle = "Title-B";
    second.url = "http://b";
    second.content = "Content-B";

    std::vector<IeControlContent> controls{first, second};
    const QString memo = service.buildMemoContent(controls);
    expectTrue(memo.contains("控件2"), "memo should include reversed first label");
    expectTrue(memo.contains("Title-B"), "memo should include second control title");
    expectTrue(memo.contains("\\newpage"), "memo should include page break");

    const QString latex = service.buildLatexContent("\\begin{doc}\n%CONTENT%\n\\end{doc}", controls);
    expectTrue(latex.contains("Content-A"), "latex should include first content");
    expectTrue(latex.contains("Content-B"), "latex should include second content");
    expectTrue(!latex.contains("%CONTENT%"), "placeholder should be replaced");

    const QString tempPath = QDir::cleanPath(QDir::current().filePath("build/phase3_test_tmp"));
    QDir().mkpath(tempPath);
    QString writeError;
    const bool writeOk = service.writeMainTex(tempPath, latex, &writeError);
    expectTrue(writeOk, "writeMainTex should succeed in test dir");
    expectTrue(writeError.isEmpty(), "writeMainTex should not return error on success");

    QFile texFile(QDir(tempPath).filePath("main.tex"));
    expectTrue(texFile.exists(), "main.tex should exist after write");
    expectTrue(texFile.open(QIODevice::ReadOnly), "main.tex should be readable");
    const QString readBack = QString::fromUtf8(texFile.readAll());
    expectTrue(readBack == latex, "main.tex content should match generated latex");
}

void testWriteDebouncePolicy()
{
    expectTrue(
        WriteDebouncePolicy::shouldWriteImmediately(false, false, 0, 250),
        "first write without previous clock should be immediate");
    expectTrue(
        !WriteDebouncePolicy::shouldWriteImmediately(true, true, 1000, 250),
        "active debounce timer should suppress immediate write");
    expectTrue(
        !WriteDebouncePolicy::shouldWriteImmediately(false, true, 100, 250),
        "recent write should suppress immediate write");
    expectTrue(
        WriteDebouncePolicy::shouldWriteImmediately(false, true, 300, 250),
        "elapsed gap over threshold should allow immediate write");
}

void testRetryBackoffPolicy()
{
    RetryBackoffPolicy policy(1000, 10000);
    expectTrue(policy.currentDelayMs() == 1000, "initial delay should equal minimum");

    policy.markFailure();
    expectTrue(policy.currentDelayMs() == 2000, "delay should double after first failure");
    policy.markFailure();
    expectTrue(policy.currentDelayMs() == 4000, "delay should double after second failure");
    policy.markFailure();
    policy.markFailure();
    policy.markFailure();
    expectTrue(policy.currentDelayMs() == 10000, "delay should cap at maximum");

    policy.reset();
    expectTrue(policy.currentDelayMs() == 1000, "reset should restore minimum delay");
}
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    testPreviewSyncService();
    testWriteDebouncePolicy();
    testRetryBackoffPolicy();

    if (g_failures == 0) {
        std::cout << "[PASS] MemoPreview tests passed" << std::endl;
        return 0;
    }

    std::cout << "[FAIL] MemoPreview tests failed: " << g_failures << std::endl;
    return 1;
}
