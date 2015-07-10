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
                uint32_t len);

bool pipeWriteN (HANDLE hPipe,
                 const void *buf,
                 uint32_t len);

bool doInThread(LPTHREAD_START_ROUTINE func, void *data);

std::vector<std::string> split(const std::string &s, char delim);

std::string normalizedPath(const std::string& path);

uint64_t currentMSecsSinceEpoch();

wchar_t *localeToWString(const std::string& src);
std::string wStringToLocale(const wchar_t *src);

wchar_t *utf8ToWString(const std::string& src);
std::string wStringToUtf8(const wchar_t *src);

std::string getBaseName(const std::string& path);
std::string getParentPath(const std::string& path);

std::string getThisDllFolder();
std::string getThisDllPath();

bool isShellExtEnabled();

} // namespace utils
} // namespace seafile

#endif // SEAFILE_EXTENSION_EXT_UTILS_H
