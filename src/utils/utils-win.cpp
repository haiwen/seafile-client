#include <windows.h>
#include <shellapi.h>
#include <wincrypt.h>
#include <glib.h>

#include <QLibrary>
#include <QPair>
#include <QString>
#include <QDir>

#include "utils/utils-win.h"

namespace utils {
namespace win {

namespace {
OSVERSIONINFOEX osver; // static variable, all zero
bool osver_failure = false;

// From http://stackoverflow.com/a/36909293/1467959 and http://yamatyuu.net/computer/program/vc2013/rtlgetversion/index.html
typedef void(WINAPI *RtlGetVersion_FUNC)(OSVERSIONINFOEXW *);
BOOL CustomGetVersion(OSVERSIONINFOEX *os)
{
    HMODULE hMod;
    RtlGetVersion_FUNC func;
#ifdef UNICODE
    OSVERSIONINFOEXW *osw = os;
#else
    OSVERSIONINFOEXW o;
    OSVERSIONINFOEXW *osw = &o;
#endif

    hMod = LoadLibrary(TEXT("ntdll.dll"));
    if (hMod) {
        func = (RtlGetVersion_FUNC)GetProcAddress(hMod, "RtlGetVersion");
        if (func == 0) {
            FreeLibrary(hMod);
            return FALSE;
        }
        ZeroMemory(osw, sizeof(*osw));
        osw->dwOSVersionInfoSize = sizeof(*osw);
        func(osw);
#ifndef UNICODE
        os->dwBuildNumber = osw->dwBuildNumber;
        os->dwMajorVersion = osw->dwMajorVersion;
        os->dwMinorVersion = osw->dwMinorVersion;
        os->dwPlatformId = osw->dwPlatformId;
        os->dwOSVersionInfoSize = sizeof(*os);
        DWORD sz = sizeof(os->szCSDVersion);
        WCHAR *src = osw->szCSDVersion;
        unsigned char *dtc = (unsigned char *)os->szCSDVersion;
        while (*src)
            *dtc++ = (unsigned char)*src++;
        *dtc = '\0';
#endif

    } else
        return FALSE;
    FreeLibrary(hMod);
    return TRUE;
}


inline bool isInitializedSystemVersion() { return osver.dwOSVersionInfoSize != 0; }
inline void initializeSystemVersion() {
    if (isInitializedSystemVersion()) {
        return;
    }
    osver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    // according to the document,
    // https://msdn.microsoft.com/en-us/library/windows/desktop/ms724451%28v=vs.85%29.aspx
    // this API will be unavailable once windows 10 is out
    if (!CustomGetVersion(&osver)) {
        qWarning("failed to get OS vesion.");
        osver_failure = true;
    }
}

inline bool _isAtLeastSystemVersion(unsigned major, unsigned minor, unsigned patch)
{
    initializeSystemVersion();
    if (osver_failure) {
        return false;
    }
#define OSVER_TO_NUM(major, minor, patch) ((major << 20) + (minor << 10) + (patch))
#define OSVER_SYS(ver) OSVER_TO_NUM(ver.dwMajorVersion, ver.dwMinorVersion, ver.wServicePackMajor)
    if (OSVER_SYS(osver) < OSVER_TO_NUM(major, minor, patch)) {
        return false;
    }
#undef OSVER_SYS
#undef OSVER_TO_NUM
    return true;
}

// compile statically
template<unsigned major, unsigned minor, unsigned patch>
inline bool isAtLeastSystemVersion()
{
    return _isAtLeastSystemVersion(major, minor, patch);
}
} // anonymous namesapce

void getSystemVersion(unsigned *major, unsigned *minor, unsigned *patch)
{
    initializeSystemVersion();
    // default to XP
    if (osver_failure) {
        *major = 5;
        *minor = 1;
        *patch = 0;
    }
    *major = osver.dwMajorVersion;
    *minor = osver.dwMinorVersion;
    *patch = osver.wServicePackMajor;
}

bool isAtLeastSystemVersion(unsigned major, unsigned minor, unsigned patch)
{
    return _isAtLeastSystemVersion(major, minor, patch);
}

bool isWindowsVistaOrHigher()
{
    return isAtLeastSystemVersion<6, 0, 0>();
}

bool isWindows7OrHigher()
{
    return isAtLeastSystemVersion<6, 1, 0>();
}

bool isWindows8OrHigher()
{
    return isAtLeastSystemVersion<6, 2, 0>();
}

bool isWindows8Point1OrHigher()
{
    return isAtLeastSystemVersion<6, 3, 0>();
}

bool isWindows10OrHigher()
{
    return isAtLeastSystemVersion<10, 0, 0>();
}

typedef HRESULT (WINAPI *GetDpiForMonitor)(HMONITOR,int,UINT *,UINT *);
typedef BOOL (WINAPI *SetProcessDPIAware)();
typedef BOOL (WINAPI *EnumDisplayMonitors)(HDC, LPCRECT, MONITORENUMPROC, LPARAM);
GetDpiForMonitor getDpiForMonitor;
SetProcessDPIAware setProcessDPIAware;
EnumDisplayMonitors enumDisplayMonitors;
typedef QPair<qreal, qreal> QDpi;

static inline QDpi monitorDPI(HMONITOR hMonitor)
{
    UINT dpiX;
    UINT dpiY;
    if (SUCCEEDED(getDpiForMonitor(hMonitor, 0, &dpiX, &dpiY)))
        return QDpi(dpiX, dpiY);
    return QDpi(0, 0);
}

static inline QDpi deviceDPI(HDC hdc)
{
    return QDpi(GetDeviceCaps(hdc, LOGPIXELSX), GetDeviceCaps(hdc, LOGPIXELSY));
}

static bool monitorData(HMONITOR hMonitor, QDpi *dpi_out)
{
    MONITORINFOEX info;
    memset(&info, 0, sizeof(MONITORINFOEX));
    info.cbSize = sizeof(MONITORINFOEX);
    if (GetMonitorInfo(hMonitor, &info) == FALSE)
        return false;

    if (QString::fromLocal8Bit(info.szDevice) == QLatin1String("WinDisc")) {
        return false;
    } else {
        QDpi dpi = monitorDPI(hMonitor);
        if (dpi.first) {
            *dpi_out = dpi;
            return true;
        } else {
            HDC hdc = CreateDC(info.szDevice, NULL, NULL, NULL);
            if (hdc) {
                *dpi_out = deviceDPI(hdc);
                DeleteDC(hdc);
                return true;
            }
        }
    }
    return false;
}

bool monitorEnumCallback(HMONITOR hMonitor, HDC hdc, LPRECT rect, LPARAM p)
{
    QDpi *data = (QDpi *)p;
    if (monitorData(hMonitor, data)) {
        // printf ("dpi = %d %d\n", (int)data->first, (int)data->second);
        return false;
    }
    return true;
}

static bool readDPI(QDpi *dpi)
{
    enumDisplayMonitors(0, 0, (MONITORENUMPROC)monitorEnumCallback, (LPARAM)dpi);
    return dpi->first != 0;
}


// QT's HDPI doesn't support non-integer scale factors, but QT_SCALE_FACTOR
// environment variable could work with them. So here we calculate the scaling
// factor (by reading the screen DPI), and update the value QT_SCALE_FACTOR with
// it.
//
// NOTE: The code below only supports single monitor. For multiple monitors we
// need to detect the dpi of each monitor and set QT_AUTO_SCREEN_SCALE_FACTOR
// accordingly. We may do that in the future.
bool fixQtHDPINonIntegerScaling()
{
    // Only do this on win8/win10
    if (!isWindows8OrHigher()) {
        return false;
    }
    // Don't overwrite the user sepcified scaling factors
    if (!qgetenv("QT_SCALE_FACTOR").isEmpty() || !qgetenv("QT_SCREEN_SCALE_FACTORS").isEmpty()) {
        return true;
    }
    // Don't overwrite the user sepcified multi-screen scaling factors
    if (!qgetenv("QT_AUTO_SCREEN_SCALE_FACTOR").isEmpty()) {
        return true;
    }

    // GetDpiForMonitor and SetProcessDPIAware are only available on win8/win10.
    //
    // See:
    //   - GetDpiForMonitor https://msdn.microsoft.com/en-us/library/windows/desktop/dn280510(v=vs.85).aspx
    //   - SetProcessDPIAware https://msdn.microsoft.com/en-us/library/windows/desktop/ms633543(v=vs.85).aspx
    QLibrary shcore_dll(QString("SHCore"));
    getDpiForMonitor = (GetDpiForMonitor)shcore_dll.resolve("GetDpiForMonitor");
    if (getDpiForMonitor == nullptr) {
        return false;
    }

    QLibrary user32_dll(QString("user32"));
    setProcessDPIAware = (SetProcessDPIAware)user32_dll.resolve("SetProcessDPIAware");
    if (setProcessDPIAware == nullptr) {
        return false;
    }

    // Turn off system scaling, otherwise we'll always see a 96 DPI virtual screen.
    if (!setProcessDPIAware()) {
        return false;
    }

    enumDisplayMonitors = (EnumDisplayMonitors)user32_dll.resolve("EnumDisplayMonitors");
    if (enumDisplayMonitors == nullptr) {
        return false;
    }

    QDpi dpi;
    if (!readDPI(&dpi)) {
        return false;
    }

    if (dpi.first <= 96) {
        return false;
    }

    // See the "DPI and the Desktop Scaling Factor" https://msdn.microsoft.com/en-us/library/windows/desktop/dn469266(v=vs.85).aspx#dpi_and_the_desktop_scaling_factor
    // Specifically, MSDN says:
    //     96 DPI = 100% scaling
    //     120 DPI = 125% scaling
    //     144 DPI = 150% scaling
    //     192 DPI = 200% scaling
    double scaling_factor = ((double)(dpi.first)) / 96.0;
    QString factor = QString::number(scaling_factor);

    // Use QT_SCREEN_SCALE_FACTORS instead of QT_SCALE_FACTOR. The latter would
    // scale the font, which has already been scaled by the system.
    //
    // See also http://lists.qt-project.org/pipermail/interest/2015-October/019242.html
    g_setenv("QT_SCREEN_SCALE_FACTORS", factor.toUtf8().data(), 1);
    // printf("set QT_SCALE_FACTOR to %s\n", factor.toUtf8().data());
    return true;
}

char *b64encode(const char *input)
{
    char buf[32767] = {0};
    DWORD retlen = 32767;
    if (!CryptBinaryToString((BYTE*) input, strlen(input), CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, buf, &retlen)) {
        return NULL;
    }
    return strdup(buf);
}

std::string getLocalPipeName(const char *pipe_name)
{
#if defined(_MSC_VER)
    const DWORD buf_char_count = 32767;
    DWORD buf_char_count_size = buf_char_count;
#else
    DWORD buf_char_count = 32767;
#endif
    char user_name_buf[buf_char_count];

#if defined(_MSC_VER)
    if (GetUserName(user_name_buf, &buf_char_count_size) == 0) {
#else
    if (GetUserName(user_name_buf, &buf_char_count) == 0) {
#endif
        qWarning("Failed to get user name, GLE=%lu\n", GetLastError());
        return pipe_name;
    } else {
        std::string ret(pipe_name);
        char *encoded = b64encode(user_name_buf);
        if (encoded) {
            ret += encoded;
            free(encoded);
        }
        return ret;
    }
}

// Qt's QProcess function cannot invoke programs that require administrator privileges,
// so we need windows api funtion to invoke the program that require adminstrator privileges.
DWORD runShellAsAdministrator(LPCSTR cmd, LPCSTR arg, int n_show)
{
    SHELLEXECUTEINFO shell_exec_info = {0};
    shell_exec_info.cbSize = sizeof(SHELLEXECUTEINFO);
    shell_exec_info.fMask = SEE_MASK_NOCLOSEPROCESS;
    shell_exec_info.hwnd = NULL;
    shell_exec_info.lpVerb = "runas";
    shell_exec_info.lpFile = cmd;
    shell_exec_info.lpParameters = arg;
    shell_exec_info.lpDirectory = NULL;
    shell_exec_info.nShow = n_show;
    shell_exec_info.hInstApp = NULL;

    BOOL ret = ShellExecuteEx(&shell_exec_info);
    WaitForSingleObject(shell_exec_info.hProcess, INFINITE);

    DWORD exit_code=0;
    GetExitCodeProcess(shell_exec_info.hProcess, &exit_code);
    CloseHandle(shell_exec_info.hProcess);
    return exit_code;
}

bool isNetworkDevice(QString path)
{
    // Shared folders are supposed to be network device.
    if (path.startsWith("//")) {
        return true;
    }

    QString root = path;
    while (!QDir(root).isRoot()) {
        root = QDir::cleanPath(root + "/..");
    }
    if (!root.endsWith("/")) {
        root += "/";
    }
    root = QDir::toNativeSeparators(root);

    LPCWSTR lpRootPathName = reinterpret_cast<LPCWSTR>(root.utf16());
    return GetDriveTypeW(lpRootPathName) == DRIVE_REMOTE;
}

} // namespace win

} // namespace utils
