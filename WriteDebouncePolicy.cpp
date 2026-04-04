#include "WriteDebouncePolicy.h"

bool WriteDebouncePolicy::shouldWriteImmediately(
    bool debounceTimerActive,
    bool lastWriteValid,
    qint64 lastWriteElapsedMs,
    qint64 minImmediateWriteGapMs)
{
    const bool firstChangeInBurst = !debounceTimerActive;
    const bool canWriteImmediately = !lastWriteValid || lastWriteElapsedMs >= minImmediateWriteGapMs;
    return firstChangeInBurst && canWriteImmediately;
}
