#include "ext-common.h"
#include <io.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <fcntl.h>
#include <psapi.h>
#include <ctype.h>
#include <userenv.h>

#include "ext-common.h"
#include "log.h"
#include "ext-utils.h"

namespace {

const int kPipeWaitTimeMSec = 1000;

} // namespace

namespace seafile {
namespace utils {

void regulatePath(char *p)
{
    if (!p)
        return;

    char *s = p;
    /* Use capitalized C/D/E, etc. */
    if (s[0] >= 'a')
        s[0] = toupper(s[0]);

    /* Use / instead of \ */
    while (*s) {
        if (*s == '\\')
            *s = '/';
        s++;
    }

    s--;
    /* strip trailing white spaces and path seperator */
    while (isspace(*s) || *s == '/') {
        *s = '\0';
        s--;
    }
}

std::string getHomeDir()
{
    static char *home;

    if (home)
        return home;

    char buf[MAX_PATH] = {'\0'};

    if (!home) {
        /* Try env variable first. */
        GetEnvironmentVariable("HOME", buf, MAX_PATH);
        if (buf[0] != '\0')
            home = strdup(buf);
    }

    if (!home) {
        /* No `HOME' ENV; Try user profile */
        HANDLE hToken = NULL;
        DWORD len = MAX_PATH;
        if (OpenProcessToken (GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
            GetUserProfileDirectory (hToken, buf, &len);
            CloseHandle(hToken);
            if (buf[0] != '\0')
                home = strdup(buf);
        }
    }

    if (home)
        regulatePath(home);
    return home ? home : "";
}

bool
doPipeWait (HANDLE pipe, OVERLAPPED *ol, DWORD len)
{
    DWORD bytes_rw, result;
    result = WaitForSingleObject (ol->hEvent, kPipeWaitTimeMSec);
    if (result == WAIT_OBJECT_0) {
        DWORD error = GetLastError();
        if (error == ERROR_IO_PENDING) {
            seaf_ext_log ("After WaitForSingleObject(), GLE = ERROR_IO_PENDING");
            if (!GetOverlappedResult(pipe, ol, &bytes_rw, false)
                || bytes_rw != len) {
                seaf_ext_log ("async read failed: %s",
                              formatErrorMessage().c_str());
                return false;
            }
        }
    } else if (result == WAIT_TIMEOUT) {
        seaf_ext_log ("connection timeout");
        return false;

    } else {
        seaf_ext_log ("failed to communicate with seafil client: %s",
                      formatErrorMessage().c_str());
        return false;
    }

    return true;
}

void resetOverlapped(OVERLAPPED *ol)
{
    ol->Offset = ol->OffsetHigh = 0;
    ResetEvent(ol->hEvent);
}

bool
checkLastError(bool *connected)
{
    DWORD last_error = GetLastError();
    if (last_error != ERROR_IO_PENDING && last_error != ERROR_SUCCESS) {
        if (last_error == ERROR_BROKEN_PIPE || last_error == ERROR_NO_DATA
            || last_error == ERROR_PIPE_NOT_CONNECTED) {
            seaf_ext_log ("connection broken with error: %s",
                          formatErrorMessage().c_str());
            *connected = false;
        } else {
            seaf_ext_log ("failed to communicate with seafile client: %s",
                          formatErrorMessage().c_str());
        }
        return false;
    }
    return true;
}

bool
pipeReadN (HANDLE pipe,
           void *buf,
           uint32_t len,
           OVERLAPPED *ol,
           bool *connected)
{
    resetOverlapped(ol);
    DWORD bytes_read;
    bool ret= ReadFile(
        pipe,                  // handle to pipe
        buf,                    // buffer to write from
        (DWORD)len,             // number of bytes to read
        NULL,                   // number of bytes read
        ol);                    // overlapped (async) IO

    if (!ret && !checkLastError(connected))
        return false;

    if (!doPipeWait (pipe, ol, (DWORD)len))
        return false;

    return true;
}

bool
pipeWriteN(HANDLE pipe,
           const void *buf,
           uint32_t len,
           OVERLAPPED *ol,
           bool *connected)
{
    resetOverlapped(ol);
    bool ret = WriteFile(
        pipe,                  // handle to pipe
        buf,                    // buffer to write from
        (DWORD)len,             // number of bytes to write
        NULL,                   // number of bytes written
        ol);                    // overlapped (async) IO

    if (!ret && !checkLastError(connected))
        return false;

    if (!doPipeWait (pipe, ol, (DWORD)len))
        return false;

    return true;
}

bool doInThread(LPTHREAD_START_ROUTINE func, void *data)
{
    DWORD tid = 0;
    HANDLE thread = CreateThread
        (NULL,                  /* security attr */
         0,                     /* stack size, 0 for default */
         func,                  /* start address */
         (void *)data,          /* param*/
         0,                     /* creation flags */
         &tid);                 /* thread ID */

    if (!thread) {
        seaf_ext_log ("failed to create thread");
        return false;
    }

    CloseHandle(thread);
    return true;
}

// http://stackoverflow.com/questions/3006229/get-a-text-from-the-error-code-returns-from-the-getlasterror-function
std::string formatErrorMessage()
{
    DWORD error_code = ::GetLastError();
    if (error_code == 0) {
        return "no error";
    }
    char buf[256] = {0};
    ::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                    NULL,
                    error_code,
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    buf,
                    sizeof(buf) - 1,
                    NULL);
    return buf;
}


} // namespace utils
} // namespace seafile
