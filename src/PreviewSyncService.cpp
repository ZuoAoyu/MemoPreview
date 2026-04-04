#include "PreviewSyncService.h"

#include <QDir>
#include <QFile>

QString PreviewSyncService::buildMemoContent(const std::vector<IeControlContent>& ieControls) const
{
    QString memoContent;
    bool firstControl = true;
    for (int i = static_cast<int>(ieControls.size()) - 1; i >= 0; --i) {
        if (!firstControl) {
            memoContent += "\\newpage\n";
        }

        const auto& ctrl = ieControls[static_cast<size_t>(i)];
        memoContent += QString("%% === 控件%1 Title: %2 URL: %3\n")
                           .arg(i + 1)
                           .arg(ctrl.htmlTitle)
                           .arg(ctrl.url);
        memoContent += ctrl.content + "\n\n";
        firstControl = false;
    }
    return memoContent;
}

QString PreviewSyncService::buildLatexContent(const QString& templateText, const std::vector<IeControlContent>& ieControls) const
{
    QString finalTemplate = templateText;
    if (finalTemplate.isEmpty()) {
        finalTemplate = "%CONTENT%";
    }

    QString latexContent = finalTemplate;
    latexContent.replace("%CONTENT%", buildMemoContent(ieControls));
    return latexContent;
}

bool PreviewSyncService::writeMainTex(const QString& workspacePath, const QString& latexContent, QString* error) const
{
    if (workspacePath.isEmpty()) {
        if (error) {
            *error = "workspacePath is empty";
        }
        return false;
    }

    const QString texFile = QDir(workspacePath).filePath("main.tex");
    QFile file(texFile);
    if (!file.open(QIODevice::WriteOnly)) {
        if (error) {
            *error = file.errorString();
        }
        return false;
    }

    file.write(latexContent.toUtf8());
    file.close();
    return true;
}
