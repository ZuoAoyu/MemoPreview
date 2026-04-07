#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QString>
#include <iostream>

#include "PreviewSyncService.h"
#include "WriteDebouncePolicy.h"
#include "RetryBackoffPolicy.h"
#include "SuperMemoIeExtractor.h"

namespace {
int g_failures = 0;

void expectTrue(bool condition, const QString& message)
{
    if (!condition) {
        std::cerr << "[FAIL] " << message.toStdString() << std::endl;
        ++g_failures;
    }
}

QString normalizedForTest(QString text)
{
    text.replace("\r\n", "\n");
    text.replace('\r', '\n');
    return text;
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

void testSuperMemoIeExtractorPreservesLatexBlocks()
{
    const QString html = QString::fromUtf8(R"(
<html>
  <body>
    <p>看涨期权给予其持有者以行权价格买入标的资产的权利。无论发生什么情况，期权的价格都不会超过标的资产价格，因此，标的资产价格是看涨期权价格的上限：</p>
    <p>\begin{equation}<br>
    c \leq S_0, \quad C \leq S_0<br>
    \tag{2-12}<br>
    \end{equation}</p>
    <p>如果以上不等式不成立，那么套利者可以购买标的资产并同时卖出看涨期权来获取无风险盈利。</p>
  </body>
</html>
)");

    const QString extracted = normalizedForTest(SuperMemoIeExtractor::extractTextFromHtml(html));
    expectTrue(
        extracted.contains("\\begin{equation}\nc \\leq S_0, \\quad C \\leq S_0\n\\tag{2-12}\n\\end{equation}"),
        "latex block should preserve single newlines inside the same paragraph");
    expectTrue(
        !extracted.contains("\\begin{equation}\n\nc \\leq S_0"),
        "latex block should not be split into multiple paragraphs");
    expectTrue(
        extracted.contains("上限：\n\n\\begin{equation}"),
        "paragraph break before latex block should be preserved");
    expectTrue(
        extracted.contains("\\end{equation}\n\n如果以上不等式不成立"),
        "paragraph break after latex block should be preserved");
}

void testSuperMemoIeExtractorPreservesPlainParagraphs()
{
    const QString html = QString::fromUtf8(R"(
<html>
  <body>
    <p>First paragraph.</p>
    <p>Second <b>paragraph</b> with inline formatting.</p>
  </body>
</html>
)");

    const QString extracted = normalizedForTest(SuperMemoIeExtractor::extractTextFromHtml(html));
    expectTrue(
        extracted.contains("First paragraph.\n\nSecond paragraph with inline formatting."),
        "plain paragraphs should keep paragraph breaks and inline spaces");
}

void testSuperMemoIeExtractorPreservesInlineSpaces()
{
    const QString html = QString::fromUtf8(R"(
<html>
  <body>
    <p>Hello <b>world</b> from <span>MemoPreview</span>.</p>
  </body>
</html>
)");

    const QString extracted = normalizedForTest(SuperMemoIeExtractor::extractTextFromHtml(html));
    expectTrue(
        extracted.contains("Hello world from MemoPreview."),
        "inline formatting should not collapse spaces between adjacent text nodes");
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
    testSuperMemoIeExtractorPreservesLatexBlocks();
    testSuperMemoIeExtractorPreservesPlainParagraphs();
    testSuperMemoIeExtractorPreservesInlineSpaces();
    testWriteDebouncePolicy();
    testRetryBackoffPolicy();

    if (g_failures == 0) {
        std::cout << "[PASS] MemoPreview tests passed" << std::endl;
        return 0;
    }

    std::cout << "[FAIL] MemoPreview tests failed: " << g_failures << std::endl;
    return 1;
}
