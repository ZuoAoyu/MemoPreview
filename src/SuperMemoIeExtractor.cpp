#include "SuperMemoIeExtractor.h"

#include <QDebug>
#include <QSet>

namespace {
constexpr UINT HTML_GETOBJECT_TIMEOUT_MS = 250;

enum class HighlightMode {
    None,
    Extract,
    Cloze,
};

struct TextAppendState {
    bool pendingSpace = false;
};

QString bstrToQString(BSTR bstr)
{
    if (!bstr) {
        return {};
    }

    const UINT len = ::SysStringLen(bstr);
    if (len == 0) {
        return {};
    }

    return QString::fromWCharArray(bstr, len);
}

bool isBlockElement(const QString& tag)
{
    static const QSet<QString> blocks = {
        "p", "div", "ul", "ol", "li",
        "h1", "h2", "h3", "h4", "h5", "h6"
    };
    return blocks.contains(tag);
}

void ensureEmptyLine(QString& result, TextAppendState& state)
{
    state.pendingSpace = false;
    if (result.isEmpty()) {
        return;
    }

    if (result.endsWith("\r\n\r\n")) {
        return;
    }

    if (result.endsWith("\r\n")) {
        result += "\r\n";
    } else {
        result += "\r\n\r\n";
    }
}

QString normalizeInlineWhitespace(QString text)
{
    text.replace("\r\n", " ");
    text.replace('\r', ' ');
    text.replace('\n', ' ');
    text.replace(QChar(0x00A0), ' ');
    return text;
}

void appendRenderedToken(QString& result,
                         const QString& text,
                         TextAppendState& state,
                         HighlightMode highlightMode,
                         bool leadingSpace,
                         bool trailingSpace)
{
    if ((state.pendingSpace || leadingSpace) &&
        !result.isEmpty() &&
        !result.endsWith("\r\n") &&
        !result.endsWith(' ')) {
        result += ' ';
    }

    switch (highlightMode) {
    case HighlightMode::Extract:
        result += QStringLiteral("\\hl{%1}").arg(text);
        break;
    case HighlightMode::Cloze:
        result += QStringLiteral("\\hc{%1}").arg(text);
        break;
    default:
        result += text;
        break;
    }

    state.pendingSpace = trailingSpace;
}

void appendNodeText(IHTMLDOMNode* node,
                    QString& result,
                    TextAppendState& state,
                    HighlightMode highlightMode = HighlightMode::None)
{
    if (!node) {
        return;
    }

    long nodeType = 0;
    if (FAILED(node->get_nodeType(&nodeType))) {
        return;
    }

    if (nodeType == 3) {
        VARIANT value;
        VariantInit(&value);
        if (SUCCEEDED(node->get_nodeValue(&value)) && value.vt == VT_BSTR && value.bstrVal) {
            const QString rawText = normalizeInlineWhitespace(bstrToQString(value.bstrVal));
            const bool leadingSpace = !rawText.isEmpty() && rawText.front().isSpace();
            const bool trailingSpace = !rawText.isEmpty() && rawText.back().isSpace();
            const QString collapsed = rawText.simplified();
            VariantClear(&value);

            if (collapsed.isEmpty()) {
                state.pendingSpace = state.pendingSpace || leadingSpace || trailingSpace;
                return;
            }

            appendRenderedToken(result, collapsed, state, highlightMode, leadingSpace, trailingSpace);
        } else {
            VariantClear(&value);
        }
        return;
    }

    if (nodeType == 1) {
        IHTMLElement* element = nullptr;
        QString tag;
        QString className;
        if (SUCCEEDED(node->QueryInterface(IID_IHTMLElement, reinterpret_cast<void**>(&element))) && element) {
            BSTR tagBstr = nullptr;
            if (SUCCEEDED(element->get_tagName(&tagBstr)) && tagBstr) {
                tag = bstrToQString(tagBstr).toLower();
                SysFreeString(tagBstr);
            }

            BSTR classBstr = nullptr;
            if (SUCCEEDED(element->get_className(&classBstr)) && classBstr) {
                className = bstrToQString(classBstr).toLower();
                SysFreeString(classBstr);
            }

            element->Release();
        }

        if (tag == "br") {
            state.pendingSpace = false;
            result += "\r\n";
            return;
        }

        if (isBlockElement(tag)) {
            ensureEmptyLine(result, state);
        }

        HighlightMode childHighlight = highlightMode;
        if (tag == "span" && !className.isEmpty()) {
            if (className.contains("clozed")) {
                childHighlight = HighlightMode::Cloze;
            } else if (className.contains("extract")) {
                childHighlight = HighlightMode::Extract;
            }
        }

        IHTMLDOMNode* child = nullptr;
        if (SUCCEEDED(node->get_firstChild(&child)) && child) {
            while (child) {
                IHTMLDOMNode* next = nullptr;
                child->get_nextSibling(&next);
                appendNodeText(child, result, state, childHighlight);
                child->Release();
                child = next;
            }
        }
        return;
    }

    IHTMLDOMNode* child = nullptr;
    if (SUCCEEDED(node->get_firstChild(&child)) && child) {
        while (child) {
            IHTMLDOMNode* next = nullptr;
            child->get_nextSibling(&next);
            appendNodeText(child, result, state, highlightMode);
            child->Release();
            child = next;
        }
    }
}

QString trimTrailingLineBreaks(QString text)
{
    while (text.endsWith("\r\n")) {
        text.chop(2);
    }
    return text;
}

QString fallbackInnerText(IHTMLElement* element)
{
    if (!element) {
        return {};
    }

    BSTR textBstr = nullptr;
    if (SUCCEEDED(element->get_innerText(&textBstr)) && textBstr) {
        const QString text = bstrToQString(textBstr);
        SysFreeString(textBstr);
        return text;
    }

    BSTR htmlBstr = nullptr;
    if (SUCCEEDED(element->get_innerHTML(&htmlBstr)) && htmlBstr) {
        const QString text = bstrToQString(htmlBstr);
        SysFreeString(htmlBstr);
        return text;
    }

    return {};
}

QString renderBodyText(IHTMLElement* body)
{
    if (!body) {
        return {};
    }

    QString result;
    IHTMLDOMNode* bodyNode = nullptr;
    if (SUCCEEDED(body->QueryInterface(IID_IHTMLDOMNode, reinterpret_cast<void**>(&bodyNode))) && bodyNode) {
        TextAppendState state;
        appendNodeText(bodyNode, result, state);
        bodyNode->Release();
    }

    return trimTrailingLineBreaks(result);
}
}

SuperMemoIeExtractor::SuperMemoIeExtractor(HWND superMemoHwnd)
    : m_superMemoHwnd(superMemoHwnd)
{
}

SuperMemoIeExtractor::~SuperMemoIeExtractor()
{
}

std::vector<IeControlContent> SuperMemoIeExtractor::extractAllIeControls()
{
    std::vector<IeControlContent> ret;
    auto hIeCtrls = enumerateIeControls(m_superMemoHwnd);
    if (hIeCtrls.empty()) {
        return ret;
    }

    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const bool comInit = SUCCEEDED(hr);
    if (hr == RPC_E_CHANGED_MODE) {
        qWarning() << "[SM_COM_APARTMENT_MISMATCH] Reusing existing COM apartment mode in worker thread.";
    } else if (FAILED(hr)) {
        qWarning() << "[SM_COM_INIT_FAILED] hr=0x" << Qt::hex << static_cast<unsigned long>(hr) << Qt::dec;
        return ret;
    }

    for (HWND hwnd : hIeCtrls) {
        IeControlContent info;
        if (extractControlContent(hwnd, info)) {
            ret.push_back(info);
        }
    }

    if (comInit) {
        CoUninitialize();
    }

    return ret;
}

std::vector<HWND> SuperMemoIeExtractor::enumerateIeControls(HWND superMemoHwnd)
{
    std::vector<HWND> result;
    if (!superMemoHwnd) {
        return result;
    }

    EnumChildWindows(superMemoHwnd, [](HWND hwnd, LPARAM lParam) -> BOOL {
        char className[256] = {0};
        GetClassNameA(hwnd, className, sizeof(className) - 1);
        if (strcmp(className, "Internet Explorer_Server") == 0) {
            auto* vec = reinterpret_cast<std::vector<HWND>*>(lParam);
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

QString SuperMemoIeExtractor::extractTextFromHtml(const QString& html)
{
    if (html.isEmpty()) {
        return {};
    }

    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const bool comInit = SUCCEEDED(hr);
    if (hr == RPC_E_CHANGED_MODE) {
        qWarning() << "[SM_TEST_COM_APARTMENT_MISMATCH] Reusing existing COM apartment mode.";
    } else if (FAILED(hr)) {
        qWarning() << "[SM_TEST_COM_INIT_FAILED] hr=0x" << Qt::hex << static_cast<unsigned long>(hr) << Qt::dec;
        return {};
    }

    QString result;
    IHTMLDocument2* document = nullptr;
    CLSID htmlDocumentClsid{};
    hr = CLSIDFromProgID(L"htmlfile", &htmlDocumentClsid);
    if (SUCCEEDED(hr)) {
        hr = CoCreateInstance(htmlDocumentClsid, nullptr, CLSCTX_INPROC_SERVER, IID_IHTMLDocument2, reinterpret_cast<void**>(&document));
    }

    if (SUCCEEDED(hr) && document) {
        IPersistStreamInit* persist = nullptr;
        if (SUCCEEDED(document->QueryInterface(IID_IPersistStreamInit, reinterpret_cast<void**>(&persist))) && persist) {
            persist->InitNew();
            persist->Release();
        }

        SAFEARRAY* array = SafeArrayCreateVector(VT_VARIANT, 0, 1);
        if (array) {
            VARIANT* entry = nullptr;
            if (SUCCEEDED(SafeArrayAccessData(array, reinterpret_cast<void**>(&entry))) && entry) {
                VariantInit(entry);
                entry->vt = VT_BSTR;
                entry->bstrVal = SysAllocString(reinterpret_cast<const OLECHAR*>(html.utf16()));
                SafeArrayUnaccessData(array);

                if (SUCCEEDED(document->write(array))) {
                    document->close();
                    result = extractDocumentContent(document);
                }
            }
            SafeArrayDestroy(array);
        }

        document->Release();
    }

    if (comInit) {
        CoUninitialize();
    }

    return result;
}

bool SuperMemoIeExtractor::extractControlContent(HWND hwnd, IeControlContent& info)
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

    info.content = getDocumentContent(hwnd, info.htmlTitle, info.url);

    const int suRefPos = info.content.lastIndexOf("#SuperMemo Reference:");
    if (suRefPos != -1) {
        info.content = info.content.left(suRefPos);
    }

    return true;
}

QString SuperMemoIeExtractor::getDocumentContent(HWND hwnd, QString& title, QString& url)
{
    const UINT message = RegisterWindowMessageA("WM_HTML_GETOBJECT");
    DWORD_PTR resultHandle = 0;
    const LRESULT sendResult = SendMessageTimeout(
        hwnd,
        message,
        0,
        0,
        SMTO_ABORTIFHUNG,
        HTML_GETOBJECT_TIMEOUT_MS,
        &resultHandle);

    if (sendResult == 0 || resultHandle == 0) {
        return {};
    }

    IHTMLDocument2* document = nullptr;
    const HRESULT hr = ObjectFromLresult(resultHandle, IID_IHTMLDocument2, 0, reinterpret_cast<void**>(&document));
    if (FAILED(hr) || !document) {
        return {};
    }

    const QString content = extractDocumentContent(document, &title, &url);
    document->Release();
    return content;
}

QString SuperMemoIeExtractor::extractDocumentContent(IHTMLDocument2* document, QString* title, QString* url)
{
    if (!document) {
        return {};
    }

    if (title) {
        BSTR titleBstr = nullptr;
        if (SUCCEEDED(document->get_title(&titleBstr)) && titleBstr) {
            *title = bstrToQString(titleBstr);
            SysFreeString(titleBstr);
        } else {
            title->clear();
        }
    }

    if (url) {
        BSTR urlBstr = nullptr;
        if (SUCCEEDED(document->get_URL(&urlBstr)) && urlBstr) {
            *url = bstrToQString(urlBstr);
            SysFreeString(urlBstr);
        } else {
            url->clear();
        }
    }

    QString result;
    IHTMLElement* body = nullptr;
    if (SUCCEEDED(document->get_body(&body)) && body) {
        // Avoid body.innerText here: once IE flattens <p> and <br> into the same newline stream,
        // we can no longer tell paragraph boundaries from multi-line LaTeX inside one paragraph.
        result = renderBodyText(body);
        if (result.isEmpty()) {
            result = fallbackInnerText(body);
        }
        body->Release();
    }

    if (!result.isEmpty()) {
        return result;
    }

    IHTMLDocument3* document3 = nullptr;
    if (SUCCEEDED(document->QueryInterface(IID_IHTMLDocument3, reinterpret_cast<void**>(&document3))) && document3) {
        IHTMLElement* root = nullptr;
        if (SUCCEEDED(document3->get_documentElement(&root)) && root) {
            result = fallbackInnerText(root);
            root->Release();
        }
        document3->Release();
    }

    return result;
}
