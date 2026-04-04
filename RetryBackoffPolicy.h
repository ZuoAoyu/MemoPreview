#ifndef RETRYBACKOFFPOLICY_H
#define RETRYBACKOFFPOLICY_H

class RetryBackoffPolicy
{
public:
    RetryBackoffPolicy(int minDelayMs, int maxDelayMs);

    int currentDelayMs() const;
    void reset();
    void markFailure();

private:
    int m_minDelayMs;
    int m_maxDelayMs;
    int m_currentDelayMs;
};

#endif // RETRYBACKOFFPOLICY_H
