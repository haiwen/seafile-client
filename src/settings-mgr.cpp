#include "seafile-applet.h"
#include "ui/tray-icon.h"
#include "rpc/rpc-client.h"

#include "settings-mgr.h"

SettingsManager::SettingsManager()
    : auto_sync_(true)
{
}

void SettingsManager::setAutoSync(bool auto_sync)
{
    if (seafApplet->rpcClient()->setAutoSync(auto_sync) < 0) {
        // Error
    }
    auto_sync_ = auto_sync;
    seafApplet->trayIcon()->setState(
        auto_sync
        ? SeafileTrayIcon::STATE_DAEMON_UP
        : SeafileTrayIcon::STATE_DAEMON_AUTOSYNC_DISABLED);
}
