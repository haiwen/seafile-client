#ifndef SEAFILE_EXTENSION_EXT_UTILS_H
#define SEAFILE_EXTENSION_EXT_UTILS_H

#include <string>
#include <vector>
#include <stdint.h>

namespace seafile {
namespace utils {

class Mutex {
public:
    Mutex();
    ~Mutex();

    void lock();
    void unlock();

private:
    Mutex(const Mutex& rhs);
    HANDLE handle_;
};

class MutexLocker {
public:
    MutexLocker(Mutex *mutex);
    ~MutexLocker();

private:
    Mutex *mu_;
};

std::string getHomeDir();

/**
 * Translate the error code of GetLastError() to a human readable message.
 */
std::string formatErrorMessage();

bool pipeReadN (HANDLE hPipe,
                void *buf,
                uint32_t len,
                OVERLAPPED *ol,
                bool *connected);

bool pipeWriteN (HANDLE hPipe,
                 const void *buf,
                 uint32_t len,
                 OVERLAPPED *ol,
                 bool *connected);

bool doInThread(LPTHREAD_START_ROUTINE func, void *data);

std::vector<std::string> split(const std::string &s, char delim);

std::string normalizedPath(const std::string& path);

uint64_t currentMSecsSinceEpoch();

std::string localeFromUtf8(const std::string& src);

std::string localeToUtf8(const std::string& src);

} // namespace utils
} // namespace seafile

#endif // SEAFILE_EXTENSION_EXT_UTILS_H
