#include <QHash>
#include <QHostInfo>

#include "utils.h"
#include "seafile-applet.h"
#include "rpc/rpc-client.h"
#include "api-utils.h"

namespace {

#if defined(Q_OS_WIN32)
const char *kOsName = "windows";
#elif defined(Q_OS_LINUX)
const char *kOsName = "linux";
#else
const char *kOsName = "mac";
#endif

} // namespace

QHash<QString, QString>
getSeafileLoginParams(const QString& computer_name, const QString& prefix)
{

    QHash<QString, QString> params;

    QString client_version = STRINGIZE(SEAFILE_CLIENT_VERSION);
    QString device_id = seafApplet->getUniqueClientId();
    QString computper = computer_name.isEmpty() ? QHostInfo::localHostName() 
        : computer_name;

    params.insert(prefix + "platform", kOsName);
    params.insert(prefix + "device_id", device_id);
    params.insert(prefix + "device_name", computer_name);
    params.insert(prefix + "client_version", client_version);
    params.insert(prefix + "platform_version", "");

    return params;
}
