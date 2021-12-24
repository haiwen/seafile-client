#ifndef SEAFILE_CLIENT_UTILS_MAC_H_
#define SEAFILE_CLIENT_UTILS_MAC_H_
#include <QtGlobal>
#ifdef Q_OS_MAC
#include <QString>
#include <vector>
#include <QByteArray>

typedef void DarkModeChangedCallback(bool value);

namespace utils {
namespace mac {
// a list for os x versions https://support.apple.com/en-us/HT201260
// release        major minor patch
// Yosemite       10    10    ?
// Mavericks      10    9     ?
// Mountain Lion  10    8     ?
// Lion           10    7     ?
void getSystemVersion(unsigned *major, unsigned *minor, unsigned *patch);
bool isAtLeastSystemVersion(unsigned major, unsigned minor, unsigned patch);
bool isOSXYosemiteOrGreater();
bool isOSXMavericksOrGreater();
bool isOSXMountainLionOrGreater();
bool isOSXLionOrGreater();

void setDockIconStyle(bool hidden);
void orderFrontRegardless(unsigned long long win_id, bool force = false);
bool get_auto_start();
void set_auto_start(bool enabled);
void copyTextToPasteboard(const QString &text);

bool is_darkmode();
void set_darkmode_watcher(DarkModeChangedCallback *cb);

QString mainBundlePath();

// load the missing part of ca certificates
std::vector<QByteArray> getSystemCaCertificates();

} // namespace mac
} // namespace utils
#else
namespace utils {
namespace mac {
inline bool isOSXYosemiteOrGreater() { return false; }
inline bool isOSXMavericksOrGreater() { return false; }
inline bool isOSXMountainLionOrGreater() { return false; }
inline bool isOSXLionOrGreater() { return false; }
} // namespace mac
} // namespace utils
#endif /* Q_OS_MAC */

#endif /* SEAFILE_CLIENT_UTILS_MAC_H_ */
