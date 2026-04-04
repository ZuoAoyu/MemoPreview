#ifndef PREVIEWSYNCSERVICE_H
#define PREVIEWSYNCSERVICE_H

#include <QString>
#include <vector>
#include "SuperMemoIeExtractor.h"

class PreviewSyncService
{
public:
    QString buildMemoContent(const std::vector<IeControlContent>& ieControls) const;
    QString buildLatexContent(const QString& templateText, const std::vector<IeControlContent>& ieControls) const;
    bool writeMainTex(const QString& workspacePath, const QString& latexContent, QString* error = nullptr) const;
};

#endif // PREVIEWSYNCSERVICE_H
