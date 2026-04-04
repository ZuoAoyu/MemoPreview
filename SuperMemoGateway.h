#ifndef SUPERMEMOGATEWAY_H
#define SUPERMEMOGATEWAY_H

#include <QVector>
#include <QString>
#include "SuperMemoWindowInfo.h"
#include "SuperMemoIeExtractor.h"

struct SuperMemoExtractionSnapshot {
    std::vector<IeControlContent> controls;
    QString contentHash;
};

class SuperMemoGateway
{
public:
    QVector<SuperMemoWindowInfo> enumerateWindows() const;
    bool isForegroundProcess(HWND hwnd) const;
    SuperMemoExtractionSnapshot extractControls(HWND hwnd) const;
};

#endif // SUPERMEMOGATEWAY_H
