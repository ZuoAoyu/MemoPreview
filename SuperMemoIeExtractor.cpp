#include "SuperMemoIeExtractor.h"



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

    // 获取title
    BSTR bstrTitle = nullptr;
    if (SUCCEEDED(pDoc->get_title(&bstrTitle)) && bstrTitle) {
        title = bstrToQString(bstrTitle);
        SysFreeString(bstrTitle);
    }

    // 获取url
    BSTR bstrURL = nullptr;
    if (SUCCEEDED(pDoc->get_URL(&bstrURL)) && bstrURL) {
        url = bstrToQString(bstrURL);
        SysFreeString(bstrURL);
    }

    // 内容优先body->innerText，再innerHTML
    IHTMLElement *pBody = nullptr;
    if (SUCCEEDED(pDoc->get_body(&pBody)) && pBody) {
        BSTR bstrText = nullptr;
        if (SUCCEEDED(pBody->get_innerText(&bstrText)) && bstrText) {
            result = bstrToQString(bstrText);
            SysFreeString(bstrText);
        } else if (SUCCEEDED(pBody->get_innerHTML(&bstrText)) && bstrText) {
            result = bstrToQString(bstrText);
            SysFreeString(bstrText);
        }
        pBody->Release();
    }

    // 再尝试documentElement
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

QString SuperMemoIeExtractor::bstrToQString(BSTR bstr)
{
    if (!bstr) return {};
    UINT len = ::SysStringLen(bstr);
    if (len == 0) return {};
    return QString::fromWCharArray(bstr, len);
}
