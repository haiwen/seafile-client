#ifndef SEAFILE_CLIENT_UTILS_MAC_H_
#define SEAFILE_CLIENT_UTILS_MAC_H_
#include <QtGlobal>
#ifdef Q_OS_MAC
#include <QString>

typedef void DarkModeChangedCallback(bool value);

namespace utils {
namespace mac {

void setDockIconStyle(bool hidden);
void orderFrontRegardless(unsigned long long win_id, bool force = false);
bool get_auto_start();
void set_auto_start(bool enabled);

bool is_darkmode();
void set_darkmode_watcher(DarkModeChangedCallback *cb);
void get_current_osx_version(unsigned *major, unsigned *minor, unsigned *patch);

QString fix_file_id_url(const QString &path);

} // namespace mac
} // namespace utils

#endif /* Q_OS_MAC */
#endif /* SEAFILE_CLIENT_UTILS_MAC_H_ */
