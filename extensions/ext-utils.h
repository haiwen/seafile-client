#ifndef SEAFILE_EXTENSION_EXT_UTILS_H
#define SEAFILE_EXTENSION_EXT_UTILS_H

#include <string>
#include <stdint.h>

namespace seafile {
namespace utils {

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


} // namespace utils
} // namespace seafile

#endif // SEAFILE_EXTENSION_EXT_UTILS_H
