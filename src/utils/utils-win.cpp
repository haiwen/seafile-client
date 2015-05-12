#include "utils/utils-win.h"
#include <windows.h>


namespace utils {
namespace win {

namespace {
OSVERSIONINFOEX osver; // static variable, all zero
bool osver_failure = false;
inline bool isInitializedSystemVersion() { return osver.dwOSVersionInfoSize != 0; }
inline void initializeSystemVersion() {
    if (isInitializedSystemVersion()) {
        return;
    }
    osver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    // according to the document,
    // https://msdn.microsoft.com/en-us/library/windows/desktop/ms724451%28v=vs.85%29.aspx
    // this API will be unavailable once windows 10 is out
    if (!GetVersionEx((LPOSVERSIONINFO)&osver)) {
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
} // namespace win
} // namespace utils
