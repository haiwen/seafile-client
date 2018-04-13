#ifndef SEAFILE_CLIENT_UTILS_WIN_H_
#define SEAFILE_CLIENT_UTILS_WIN_H_
#include <QtGlobal>
#ifdef Q_OS_WIN32

namespace utils {
namespace win {
// a list for windows versions https://msdn.microsoft.com/en-us/library/windows/desktop/ms724833%28v=vs.85%29.aspx
// Windows Relaese        major    minor   patch(SP)
// windows 10:            10,      0,      ?
// windows 8.1:           6,       3,      ?
// windows 2012 R2:       6,       3,      ?
// windows 8:             6,       2,      ?
// windows 2012:          6,       2,      ?
// windows 7:             6,       1,      ?
// windows 2008 R2:       6,       1,      ?
// windows Vista:         6,       0,      ?
// windows 2008:          6,       0,      ?
// windows 2003 R2:       5,       2,      ?
// windows 2003:          5,       2,      ?
// windows XP x64:        5,       2,      ?
// windows XP:            5,       1,      ?
// windows 2000:          5,       0,      ?
void getSystemVersion(unsigned *major, unsigned *minor, unsigned *patch);
bool isAtLeastSystemVersion(unsigned major, unsigned minor, unsigned patch);

bool isWindowsVistaOrGreater();
bool isWindows7OrGreater();
bool isWindows8OrGreater();
bool isWindows8Point1OrGreater();
bool isWindows10OrHigher();
bool fixQtHDPINonIntegerScaling();
} // namespace win
} // namespace utils
#else
namespace utils {
namespace win {
inline bool isWindowsVistaOrGreater() { return false; }
inline bool isWindows7OrGreater() { return false; }
inline bool isWindows8OrGreater() { return false; }
inline bool isWindows8Point1OrGreater() { return false; }
inline bool isWindows10OrHigher() { return false; }

std::string getLocalPipeName(const char *pipeName);
} // namespace win
} // namespace utils
#endif


#endif // SEAFILE_CLIENT_UTILS_WIN_H_
