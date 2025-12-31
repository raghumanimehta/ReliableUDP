#include "utils.hpp"
#include <poll.h>
#include <errno.h>
#include "logger.cpp"

POOL_STATE waitForRead(int fd, uint64_t timeoutMs)
{
    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLIN;
    pfd.revents = 0;

    int ret = poll(&pfd, 1, timeoutMs);
    if (ret < 0) {
        if (errno == EINTR) return INTR;
        LOG_ERROR("Poll error: " + std::string(strerror(errno)));
        return FAIL;
    } else if (ret > 0 && (pfd.revents & POLLIN)) {
        return SUCCESS;
    } else {
        return TIMEOUT;  
    }
}

POOL_STATE waitForReadWithRetry(int fd, uint64_t timeoutMs, uint8_t maxRetries)
{
    uint8_t retries = 0;
    POOL_STATE pollState;

    while (retries < maxRetries)
    {
        pollState = waitForRead(fd, timeoutMs);
        if (pollState == SUCCESS) return SUCCESS;
        if (pollState == INTR) continue;  
        if (pollState == FAIL) return FAIL;
        retries++;  
    }
    return TIMEOUT;
}