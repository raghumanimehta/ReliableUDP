#include <cstdint>
#ifndef RELIABLE_UDP_UTILS_HPP
#define RELIABLE_UDP_UTILS_HPP
// Waits for 'fd' to be readable.
//
// timeoutMs - timeout per poll() call in milliseconds.
// maxRetries - number of poll retries; 0 means "wait indefinitely".
// Returns:  1 if readable, 0 on timeout (retries exhausted), -1 on error.

enum POOL_STATE {
    SUCCESS,
    FAIL,
    INTR,
    TIMEOUT
};

POOL_STATE waitForRead(int fd, uint64_t timeoutMs);
POOL_STATE waitForReadWithRetry(int fd, uint64_t timeoutMs, uint8_t maxRetries);

#endif