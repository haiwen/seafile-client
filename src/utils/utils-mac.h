#ifndef SEAFILE_CLIENT_UTILS_MAC_H_
#include <QtGlobal>
#ifdef Q_OS_MAC
#include <QString>

namespace utils {
namespace mac {

void setDockIconStyle(bool);
bool get_auto_start();
void set_auto_start(bool enabled);

QString fix_file_id_url(const QString &path);

} // namespace mac
} // namespace utils

#endif /* Q_OS_MAC */
#endif /* SEAFILE_CLIENT_UTILS_MAC_H_ */
