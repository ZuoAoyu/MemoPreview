#include "RetryBackoffPolicy.h"

#include <QtGlobal>

RetryBackoffPolicy::RetryBackoffPolicy(int minDelayMs, int maxDelayMs)
    : m_minDelayMs(minDelayMs),
      m_maxDelayMs(maxDelayMs),
      m_currentDelayMs(minDelayMs)
{
}

int RetryBackoffPolicy::currentDelayMs() const
{
    return m_currentDelayMs;
}

void RetryBackoffPolicy::reset()
{
    m_currentDelayMs = m_minDelayMs;
}

void RetryBackoffPolicy::markFailure()
{
    m_currentDelayMs = qMin(m_currentDelayMs * 2, m_maxDelayMs);
}
