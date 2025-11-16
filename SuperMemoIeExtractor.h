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
    HWND hwnd;
    QString className;
    QString windowText;
    RECT rect;
    bool isVisible;
    QString htmlTitle;
    QString url;
    QString content; // 抓取到的内容
};

class SuperMemoIeExtractor
{
public:
    explicit SuperMemoIeExtractor(HWND superMemoHwnd);
    ~SuperMemoIeExtractor();

    // 抓取所有IE控件内容（线程安全，可在后台线程调用）
    std::vector<IeControlContent> extractAllIeControls();

    static std::vector<HWND> enumerateIeControls(HWND superMemoHwnd);

    // 检测指定 SuperMemo 主窗口下 IE 控件数量
    static int countIeControls(HWND superMemoHwnd);

private:
    HWND m_superMemoHwnd;

    // 主要入口：抓取单个控件内容
    static bool extractControlContent(HWND hwnd, IeControlContent &info);

    // 获取 IHTMLDocument2 内容相关
    static QString getDocumentContent(HWND hwnd, QString &title, QString &url);
};

#endif // SUPERMEMOIEEXTRACTOR_H
