#ifndef SEAFILE_CLIENT_UTILS_MAC_H_
#include <QtGlobal>
#ifdef Q_WS_MAC
#include <QString>

namespace utils {
namespace mac {

int isHiDPI(void);
double getScaleFactor(void);
void setDockIconStyle(bool);
bool get_auto_start();
void set_auto_start(bool enabled);

QString get_path_from_fileId_url(const QString &url);

} // namespace mac
} // namespace utils

#endif /* Q_WS_MAC */
#endif /* SEAFILE_CLIENT_UTILS_MAC_H_ */
