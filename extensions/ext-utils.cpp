#include "ext-common.h"
#include <io.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <fcntl.h>
#include <psapi.h>
#include <ctype.h>
#include <userenv.h>

#include <sstream>
#include <algorithm>
#include <memory>

#include "log.h"
#include "ext-utils.h"
#include "shell-ext.h"

namespace {

const int kPipeWaitTimeMSec = 1000;

class OverLappedWrapper
{
public:
    OverLappedWrapper() {
        memset(&ol_, 0, sizeof(ol_));
        ol_.Offset = ol_.OffsetHigh = 0;
        HANDLE h_ev = CreateEvent
            (NULL,                  /* security attribute */
             FALSE,                 /* manual reset */
             FALSE,                 /* initial state  */
             NULL);                 /* event name */

        ol_.hEvent = h_ev;
    }

    ~OverLappedWrapper() {
        CloseHandle(ol_.hEvent);
    }

    OVERLAPPED *get() { return &ol_; }

private:
    OVERLAPPED ol_;
};

} // namespace

namespace seafile {
namespace utils {


Mutex::Mutex()
{
    handle_ = CreateMutex
        (NULL,                  /* securitry attr */
         FALSE,                 /* own the mutex immediately after create */
         NULL);                 /* name */
}

Mutex::~Mutex()
{
    CloseHandle(handle_);
}

void Mutex::lock()
{
    while (1) {
        DWORD ret = WaitForSingleObject(handle_, INFINITE);
        if (ret == WAIT_OBJECT_0)
            return;
    }
}

void Mutex::unlock()
{
    ReleaseMutex(handle_);
}

MutexLocker::MutexLocker(Mutex *mutex)
    : mu_(mutex)
{
    mu_->lock();
}

MutexLocker::~MutexLocker()
{
    mu_->unlock();
}


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
        if (!GetOverlappedResult(pipe, ol, &bytes_rw, false)
            || bytes_rw != len) {
            seaf_ext_log ("async read failed: %s",
                            formatErrorMessage().c_str());
            return false;
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

bool
checkLastError()
{
    DWORD last_error = GetLastError();
    if (last_error != ERROR_IO_PENDING && last_error != ERROR_SUCCESS) {
        if (last_error == ERROR_BROKEN_PIPE || last_error == ERROR_NO_DATA
            || last_error == ERROR_PIPE_NOT_CONNECTED) {
            seaf_ext_log ("connection broken with error: %s",
                          formatErrorMessage().c_str());
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
           uint32_t len)
{
    OverLappedWrapper ol;
    bool ret= ReadFile(
        pipe,                  // handle to pipe
        buf,                    // buffer to write from
        (DWORD)len,             // number of bytes to read
        NULL,                   // number of bytes read
        ol.get());              // overlapped (async) IO

    if (!ret && !checkLastError()) {
        return false;
    }

    if (!doPipeWait (pipe, ol.get(), (DWORD)len)) {
        return false;
    }

    return true;
}

bool
pipeWriteN(HANDLE pipe,
           const void *buf,
           uint32_t len)
{
    OverLappedWrapper ol;
    bool ret = WriteFile(
        pipe,                   // handle to pipe
        buf,                    // buffer to write from
        (DWORD)len,             // number of bytes to write
        NULL,                   // number of bytes written
        ol.get());              // overlapped (async) IO

    if (!ret && !checkLastError()) {
        return false;
    }

    if (!doPipeWait(pipe, ol.get(), (DWORD)len))
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

std::vector<std::string> split(const std::string &s, char delim)
{
    std::vector<std::string> elems;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim) && !item.empty()) {
        elems.push_back(item);
    }
    return elems;
}

std::string normalizedPath(const std::string& path)
{
    std::string p = path;
    std::replace(p.begin(), p.end(), '\\', '/');
    while (!p.empty() && p[p.size() - 1] == '/') {
        p = p.substr(0, p.size() - 1);
    }
    return p;
}

uint64_t currentMSecsSinceEpoch()
{
    SYSTEMTIME st;
    FILETIME ft;
    GetSystemTime(&st);
    SystemTimeToFileTime(&st, &ft);

    ULARGE_INTEGER u;
    u.LowPart  = ft.dwLowDateTime;
    u.HighPart = ft.dwHighDateTime;

    // See http://stackoverflow.com/questions/6161776/convert-windows-filetime-to-second-in-unix-linux
#define TICKS_PER_MSEC 10000
#define EPOCH_DIFFERENCE 11644473600000LL
    uint64_t temp;
    temp = u.QuadPart / TICKS_PER_MSEC; //convert from 100ns intervals to milli seconds;
    temp = temp - EPOCH_DIFFERENCE; //subtract number of seconds between epochs
    return temp;
}

std::string splitPath(const std::string& path, int *pos)
{
    if (path.size() == 0) {
        return "";
    }

    std::string p = normalizedPath(path);
    while (p.size() > 1 && p[-1] == '/') {
        p = p.substr(0, p.size() - 1);
    }
    if (p.size() == 1) {
        return p;
    }

    *pos = p.rfind("/");
    return p;
}


std::string getParentPath(const std::string& path)
{
    int pos;
    std::string p = splitPath(path, &pos);
    if (p.size() <= 1) {
        return p;
    }

    if (pos == -1)
        return "";
    if (pos == 0)
        return "/";
    return p.substr(0, pos);
}

std::string getBaseName(const std::string& path)
{
    int pos;
    std::string p = splitPath(path, &pos);
    if (p.size() <= 1) {
        return p;
    }

    if (pos == -1) {
        return p;
    }
    return p.substr(pos, p.size() - pos);
}

std::string getThisDllPath()
{
    static char module_filename[MAX_PATH] = { 0 };

    if (module_filename[0] == '\0') {
        DWORD module_size;
        module_size = GetModuleFileName(
            g_hmodThisDll, module_filename, MAX_PATH);
        if (!module_size)
            return "";

        normalizedPath(module_filename);
    }

    return module_filename;
}

std::string getThisDllFolder()
{
    std::string dll = getThisDllPath();

    return dll.empty() ? "" : getParentPath(dll);
}

wchar_t *localeToWString(const std::string& src)
{
    wchar_t dst[4096];
    int len;

    len = MultiByteToWideChar
        (CP_ACP,                /* multibyte code page */
         0,                     /* flags */
         src.c_str(),           /* src */
         -1,                    /* src len, -1 for all includes \0 */
         dst,                   /* dst */
         sizeof(dst) / sizeof(wchar_t));          /* dst buf len */

    if (len <= 0) {
        return NULL;
    }

    return wcsdup(dst);
}


std::string wStringToLocale(const wchar_t *src)
{
    char dst[4096];
    int len;

    len = WideCharToMultiByte
        (CP_ACP,        /* multibyte code page */
         0,             /* flags */
         src,           /* src */
         -1,            /* src len, -1 for all includes \0 */
         dst,           /* dst */
         sizeof(dst),   /* dst buf len */
         NULL,          /* default char */
         NULL);         /* BOOL flag indicates default char is used */

    if (len <= 0) {
        return "";
    }

    return dst;
}

std::string wStringToUtf8(const wchar_t *src)
{
    char dst[4096];
    int len;

    len = WideCharToMultiByte
        (CP_UTF8,               /* multibyte code page */
         0,                     /* flags */
         src,                   /* src */
         -1,                    /* src len, -1 for all includes \0 */
         dst,                   /* dst */
         sizeof(dst),           /* dst buf len */
         NULL,                  /* default char */
         NULL);                 /* BOOL flag indicates default char is used */

    if (len <= 0) {
        return "";
    }

    return dst;
}

wchar_t *utf8ToWString(const std::string& src)
{
    wchar_t dst[4096];
    int len;

    len = MultiByteToWideChar
        (CP_UTF8,                        /* multibyte code page */
         0,                              /* flags */
         src.c_str(),                    /* src */
         -1,                             /* src len, -1 for all includes \0 */
         dst,                            /* dst */
         sizeof(dst) / sizeof(wchar_t)); /* dst buf len */

    if (len <= 0) {
        return NULL;
    }

    return wcsdup(dst);
}

bool isShellExtEnabled()
{
    // TODO: This function is called very frequently. Consider caching the
    // result in memory for a few seconds, and only query the registry after
    // that.
    HKEY root = HKEY_CURRENT_USER;
    HKEY parent_key;
    wchar_t *software_seafile = localeToWString("Software\\Seafile");
    LONG result = RegOpenKeyExW(root,
                                software_seafile,
                                0L,
                                KEY_ALL_ACCESS,
                                &parent_key);
    free(software_seafile);
    if (result != ERROR_SUCCESS) {
        return true;
    }

    char buf[MAX_PATH] = {0};
    DWORD len = sizeof(buf);
    wchar_t *shell_ext_disabled = localeToWString("ShellExtDisabled");
    result = RegQueryValueExW (parent_key,
                               shell_ext_disabled,
                               NULL,             /* reserved */
                               NULL,             /* output type */
                               (LPBYTE)buf,      /* output data */
                               &len);            /* output length */
    RegCloseKey(parent_key);
    free(shell_ext_disabled);

    return result != ERROR_SUCCESS;
}


} // namespace utils
} // namespace seafile
