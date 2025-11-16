#include "SuperMemoIeExtractor.h"
#include <QSet>
#include <QRegularExpression>

// BSTR->QString
QString bstrToQString(BSTR bstr)
{
    if (!bstr) return {};
    UINT len = ::SysStringLen(bstr);
    if (len == 0) return {};
    return QString::fromWCharArray(bstr, len);
}

// 判断是不是块级元素（可按需再补充）
bool isBlockElement(const QString &tag)
{
    static const QSet<QString> blocks = {
        "p", "div", "ul", "ol", "li",
        "h1", "h2", "h3", "h4", "h5", "h6"
    };
    return blocks.contains(tag);
}

// 确保 result 结尾至少有一个空行（两次 \r\n）
void ensureEmptyLine(QString &result)
{
    if (result.isEmpty())
        return;

    if (result.endsWith("\r\n\r\n"))
        return;

    if (result.endsWith("\r\n"))
        result += "\r\n";
    else
        result += "\r\n\r\n";
}

void appendNodeText(IHTMLDOMNode *pNode, QString &result)
{
    if (!pNode) return;

    long nodeType = 0;
    if (FAILED(pNode->get_nodeType(&nodeType)))
        return;

    // --- 文本节点 ---
    if (nodeType == 3) { // NODE_TEXT
        VARIANT v;
        VariantInit(&v);
        if (SUCCEEDED(pNode->get_nodeValue(&v)) && v.vt == VT_BSTR && v.bstrVal) {
            QString text = bstrToQString(v.bstrVal);
            VariantClear(&v);

            // 原始源码里的换行在 HTML 渲染里多半只是空白，这里压成空格
            text.replace("\r", " ");
            text.replace("\n", " ");

            // 合并多余空白
            text = text.replace(QRegularExpression{"\\s+"}, " ");

            QString trimmed = text.trimmed();
            if (trimmed.isEmpty())
                return;

            // 看看父节点是不是 <body>，如果是，就把这段文本当成一个“段落”
            QString parentTag;
            IHTMLDOMNode *pParent = nullptr;
            if (SUCCEEDED(pNode->get_parentNode(&pParent)) && pParent) {
                IHTMLElement *pParentElem = nullptr;
                if (SUCCEEDED(pParent->QueryInterface(IID_IHTMLElement, (void**)&pParentElem)) && pParentElem) {
                    BSTR bTag = nullptr;
                    if (SUCCEEDED(pParentElem->get_tagName(&bTag)) && bTag) {
                        parentTag = bstrToQString(bTag).toLower();
                        SysFreeString(bTag);
                    }
                    pParentElem->Release();
                }
                pParent->Release();
            }

            bool parentIsBody = (parentTag == "body");

            if (parentIsBody) {
                // 裸文本直接在 body 下：把它当成自己的段落
                ensureEmptyLine(result);
            }

            // 如果前面已经有内容且最后不是空格/换行，补一个空格（避免黏在一起）
            if (!result.isEmpty() && !result.endsWith(" ") && !result.endsWith("\n"))
                result += " ";

            result += trimmed;
        } else {
            VariantClear(&v);
        }
        return;
    }

    // --- 元素节点 ---
    if (nodeType == 1) { // NODE_ELEMENT
        IHTMLElement *pElem = nullptr;
        QString tag;

        if (SUCCEEDED(pNode->QueryInterface(IID_IHTMLElement, (void**)&pElem)) && pElem) {
            BSTR bTag = nullptr;
            if (SUCCEEDED(pElem->get_tagName(&bTag)) && bTag) {
                tag = bstrToQString(bTag).toLower();
                SysFreeString(bTag);
            }
            pElem->Release();
        }

        // <br>：行内换行
        if (tag == "br") {
            result += "\r\n";
            return;
        }

        bool isParagraph = (tag == "p");
        bool isListItem  = (tag == "li");

        // 段落/列表项开始前，确保有一个空行
        if (isParagraph || isListItem) {
            ensureEmptyLine(result);
        }

        if (isListItem) {
            // 简单的项目符号，按需要可删掉或换别的
            result += "- ";
        }

        // 递归子节点
        IHTMLDOMNode *pChild = nullptr;
        if (SUCCEEDED(pNode->get_firstChild(&pChild)) && pChild) {
            while (pChild) {
                IHTMLDOMNode *pNext = nullptr;
                pChild->get_nextSibling(&pNext);

                appendNodeText(pChild, result);

                pChild->Release();
                pChild = pNext;
            }
        }

        if (isListItem) {
            // 每个 li 末尾换行
            if (!result.endsWith("\r\n"))
                result += "\r\n";
        }

        return;
    }

    // 其他节点类型：只递归子节点
    IHTMLDOMNode *pChild = nullptr;
    if (SUCCEEDED(pNode->get_firstChild(&pChild)) && pChild) {
        while (pChild) {
            IHTMLDOMNode *pNext = nullptr;
            pChild->get_nextSibling(&pNext);

            appendNodeText(pChild, result);

            pChild->Release();
            pChild = pNext;
        }
    }
}


SuperMemoIeExtractor::SuperMemoIeExtractor(HWND superMemoHwnd)
:m_superMemoHwnd(superMemoHwnd)
{

}

SuperMemoIeExtractor::~SuperMemoIeExtractor()
{

}

std::vector<IeControlContent> SuperMemoIeExtractor::extractAllIeControls()
{
    std::vector<IeControlContent> ret;
    auto hIeCtrls = enumerateIeControls(m_superMemoHwnd);
    if (hIeCtrls.empty()) return ret;

    // 保证COM初始化只在本线程一次
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    bool comInit = SUCCEEDED(hr);

    for (HWND hwnd : hIeCtrls) {
        IeControlContent info;
        if (extractControlContent(hwnd, info))
            ret.push_back(info);
    }
    if (comInit) CoUninitialize();
    return ret;
}

std::vector<HWND> SuperMemoIeExtractor::enumerateIeControls(HWND superMemoHwnd)
{
    std::vector<HWND> result;
    if (!superMemoHwnd) return result;

    EnumChildWindows(superMemoHwnd, [](HWND hwnd, LPARAM lParam) -> BOOL {
        char className[256] = {0};
        GetClassNameA(hwnd, className, sizeof(className) - 1);
        if (strcmp(className, "Internet Explorer_Server") == 0) {
            auto vec = reinterpret_cast<std::vector<HWND>*>(lParam);
            vec->push_back(hwnd);
        }
        return TRUE;
    }, reinterpret_cast<LPARAM>(&result));
    return result;
}

int SuperMemoIeExtractor::countIeControls(HWND superMemoHwnd)
{
    return enumerateIeControls(superMemoHwnd).size();
}

bool SuperMemoIeExtractor::extractControlContent(HWND hwnd, IeControlContent &info)
{
    char className[256] = {0};
    char winText[256] = {0};
    GetClassNameA(hwnd, className, sizeof(className) - 1);
    GetWindowTextA(hwnd, winText, sizeof(winText) - 1);

    info.hwnd = hwnd;
    info.className = QString::fromLocal8Bit(className);
    info.windowText = QString::fromLocal8Bit(winText);
    info.isVisible = !!IsWindowVisible(hwnd);
    GetWindowRect(hwnd, &info.rect);

    // 获取HTML文档内容
    info.content = getDocumentContent(hwnd, info.htmlTitle, info.url);

    // 对于一个 SuperMemo Element，如果为其添加了 Reference，Element 末尾会有相关的字符串，需要在编译 PDF 文档时将其去除
    int suRefPos = info.content.lastIndexOf("#SuperMemo Reference:");
    if (suRefPos != -1) {
        info.content = info.content.left(suRefPos);
    }

    return true;
}

QString SuperMemoIeExtractor::getDocumentContent(HWND hwnd, QString &title, QString &url)
{
    QString result;

    UINT nMsg = RegisterWindowMessageA("WM_HTML_GETOBJECT");
    DWORD_PTR dwRes = 0;
    LRESULT lRes = SendMessageTimeout(hwnd, nMsg, 0, 0, SMTO_ABORTIFHUNG, 1000, &dwRes);

    if (dwRes == 0) return {};

    IHTMLDocument2 *pDoc = nullptr;
    HRESULT hr = ObjectFromLresult(dwRes, IID_IHTMLDocument2, 0, (void **)&pDoc);
    if (!SUCCEEDED(hr) || !pDoc) return {};

    // title
    BSTR bstrTitle = nullptr;
    if (SUCCEEDED(pDoc->get_title(&bstrTitle)) && bstrTitle) {
        title = bstrToQString(bstrTitle);
        SysFreeString(bstrTitle);
    }

    // url
    BSTR bstrURL = nullptr;
    if (SUCCEEDED(pDoc->get_URL(&bstrURL)) && bstrURL) {
        url = bstrToQString(bstrURL);
        SysFreeString(bstrURL);
    }

    // 内容：DOM 遍历
    IHTMLElement *pBody = nullptr;
    if (SUCCEEDED(pDoc->get_body(&pBody)) && pBody) {
        IHTMLDOMNode *pBodyNode = nullptr;
        if (SUCCEEDED(pBody->QueryInterface(IID_IHTMLDOMNode, (void**)&pBodyNode)) && pBodyNode) {
            appendNodeText(pBodyNode, result);
            pBodyNode->Release();
        }

        // 兜底逻辑：如果 result 还是空，就用 innerText
        if (result.isEmpty()) {
            BSTR bstrText = nullptr;
            if (SUCCEEDED(pBody->get_innerText(&bstrText)) && bstrText) {
                result = bstrToQString(bstrText);
                SysFreeString(bstrText);
            } else if (SUCCEEDED(pBody->get_innerHTML(&bstrText)) && bstrText) {
                result = bstrToQString(bstrText);
                SysFreeString(bstrText);
            }
        }

        pBody->Release();
    }

    // 再尝试 documentElement（完全兜底）
    if (result.isEmpty()) {
        IHTMLDocument3 *pDoc3 = nullptr;
        if (SUCCEEDED(pDoc->QueryInterface(IID_IHTMLDocument3, (void**)&pDoc3)) && pDoc3) {
            IHTMLElement *pRoot = nullptr;
            if (SUCCEEDED(pDoc3->get_documentElement(&pRoot)) && pRoot) {
                BSTR bstrText = nullptr;
                if (SUCCEEDED(pRoot->get_innerText(&bstrText)) && bstrText) {
                    result = bstrToQString(bstrText);
                    SysFreeString(bstrText);
                }
                pRoot->Release();
            }
            pDoc3->Release();
        }
    }

    pDoc->Release();
    return result;
}

