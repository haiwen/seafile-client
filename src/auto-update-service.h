#ifndef SEAFILE_CLIENT_AUTO_UPDATE_SERVICE_H
#define SEAFILE_CLIENT_AUTO_UPDATE_SERVICE_H

#include <QObject>
#include <QString>

#include "utils/singleton.h"

// Auto update seafile client program. Only used on windows.
class AutoUpdateService : public QObject
{
    SINGLETON_DEFINE(AutoUpdateService)
    Q_OBJECT

public:
    AutoUpdateService(QObject *parent = 0);

    bool shouldSupportAutoUpdate() const;

    void setRequestParams();
    bool autoUpdateEnabled() const;
    void setAutoUpdateEnabled(bool enabled);
    uint updateCheckInterval() const;
    void setUpdateCheckInterval(uint interval_in_seconds);

    void start();
    void stop();

    void checkUpdate();
    void checkAndInstallUpdate();
    void checkUpdateWithoutUI();

private:
    QString getAppcastURI();
};

#endif // SEAFILE_CLIENT_AUTO_UPDATE_SERVICE_H
