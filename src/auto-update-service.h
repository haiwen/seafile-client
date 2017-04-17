#ifndef SEAFILE_CLIENT_AUTO_UPDATE_SERVICE_H
#define SEAFILE_CLIENT_AUTO_UPDATE_SERVICE_H

#include <QObject>
#include <QString>

#include "utils/singleton.h"

class AutoUpdateAdapter;

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

    void start();
    void stop();

    void checkUpdate();

private:
    QString getAppcastURI();
    AutoUpdateAdapter *adapter_;
};

#endif // SEAFILE_CLIENT_AUTO_UPDATE_SERVICE_H
