#ifndef SUPERMEMOIEEXTRACTOR_H
#define SUPERMEMOIEEXTRACTOR_H

#include <windows.h>
#include <vector>
#include <QString>
#include <QStringList>
#include <oleacc.h>
#include <mshtml.h>

// IE控件信息结构
struct IeControlContent {
    HWND hwnd = nullptr;
    QString className;
    QString windowText;
    RECT rect{};
    bool isVisible = false;
    QString htmlTitle;
    QString url;
    QString content; // 抓取到的内容
    quint32 bodyHtmlHash = 0;
    qsizetype bodyHtmlLength = 0;
};

class SuperMemoIeExtractor
{
public:
    explicit SuperMemoIeExtractor(HWND superMemoHwnd);
    ~SuperMemoIeExtractor();

    // 抓取所有IE控件内容（线程安全，可在后台线程调用）
    std::vector<IeControlContent> extractAllIeControls(const std::vector<IeControlContent>& previousControls = {});

    static std::vector<HWND> enumerateIeControls(HWND superMemoHwnd);

    // 检测指定 SuperMemo 主窗口下 IE 控件数量
    static int countIeControls(HWND superMemoHwnd);

    // 供测试/诊断使用：直接从 HTML 字符串按 DOM 结构提取正文文本
    static QString extractTextFromHtml(const QString& html);

private:
    HWND m_superMemoHwnd;

    // 主要入口：抓取单个控件内容
    static bool extractControlContent(HWND hwnd, const IeControlContent* previousControl, IeControlContent &info);

    // 获取 IHTMLDocument2 内容相关
    static QString getDocumentContent(HWND hwnd,
                                      const IeControlContent* previousControl,
                                      QString &title,
                                      QString &url,
                                      quint32* bodyHtmlHash = nullptr,
                                      qsizetype* bodyHtmlLength = nullptr);
    static QString extractDocumentContent(IHTMLDocument2* document,
                                          const IeControlContent* previousControl = nullptr,
                                          QString* title = nullptr,
                                          QString* url = nullptr,
                                          quint32* bodyHtmlHash = nullptr,
                                          qsizetype* bodyHtmlLength = nullptr);
};

#endif // SUPERMEMOIEEXTRACTOR_H
