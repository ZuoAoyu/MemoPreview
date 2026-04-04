#ifndef WRITEDEBOUNCEPOLICY_H
#define WRITEDEBOUNCEPOLICY_H

#include <QtGlobal>

class WriteDebouncePolicy
{
public:
    static bool shouldWriteImmediately(
        bool debounceTimerActive,
        bool lastWriteValid,
        qint64 lastWriteElapsedMs,
        qint64 minImmediateWriteGapMs);
};

#endif // WRITEDEBOUNCEPOLICY_H
