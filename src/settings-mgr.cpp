#include "seafile-applet.h"
#include "ui/tray-icon.h"
#include "settings-mgr.h"

#include "rpc/requests.h"

SettingsManager::SettingsManager()
    : auto_sync_(true)
{
}

void SettingsManager::setAutoSync(bool auto_sync)
{
    SetAutoSyncRequest *req = new SetAutoSyncRequest(auto_sync);
    req->send();

    connect(req, SIGNAL(success(bool)), this, SLOT(onSetAutoSyncFinished(bool)));
}

void SettingsManager::onSetAutoSyncFinished(bool auto_sync)
{
    qDebug("%s auto sync success", auto_sync ? "enable" : "disable");
    auto_sync_ = auto_sync;
    seafApplet->trayIcon()->setState(
        auto_sync
        ? SeafileTrayIcon::STATE_DAEMON_UP
        : SeafileTrayIcon::STATE_DAEMON_AUTOSYNC_DISABLED);
}
