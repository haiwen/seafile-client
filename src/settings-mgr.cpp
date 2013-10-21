#include <QSettings>
#include "seafile-applet.h"
#include "ui/tray-icon.h"
#include "settings-mgr.h"
#include "rpc/rpc-client.h"
#include "utils/utils.h"

namespace {

const char *kHideMainWindowWhenStarted = "hideMainWindowWhenStarted";
const char *kBehaviorGroup = "Behavior";

} // namespace


SettingsManager::SettingsManager()
    : auto_sync_(true),
      bubbleNotifycation_(true),
      autoStart_(false),
      transferEncrypted_(true),
      maxDownloadRatio_(0),
      maxUploadRatio_(0)
{
}

void SettingsManager::loadSettings()
{
    QString str;
    int value;

    if (seafApplet->rpcClient()->seafileGetConfig("notify_sync", &str) >= 0)
        bubbleNotifycation_ = (str == "off") ? false : true;

    if (seafApplet->rpcClient()->ccnetGetConfig("encrypt_channel", &str) >= 0)
        transferEncrypted_ = (str == "off") ? false : true;

    if (seafApplet->rpcClient()->seafileGetConfigInt("download_limit", &value) >= 0)
        maxDownloadRatio_ = value >> 10;

    if (seafApplet->rpcClient()->seafileGetConfigInt("upload_limit", &value) >= 0)
        maxUploadRatio_ = value >> 10;

    autoStart_ = get_seafile_auto_start();
}

void SettingsManager::setAutoSync(bool auto_sync)
{
    if (seafApplet->rpcClient()->setAutoSync(auto_sync) < 0) {
        // Error
        return;
    }
    auto_sync_ = auto_sync;
    seafApplet->trayIcon()->setState(
        auto_sync
        ? SeafileTrayIcon::STATE_DAEMON_UP
        : SeafileTrayIcon::STATE_DAEMON_AUTOSYNC_DISABLED);
}

void SettingsManager::setNotify(bool notify)
{
    qDebug("%s", __func__);
    if (bubbleNotifycation_ != notify) {
        if (seafApplet->rpcClient()->seafileSetConfig("notify_sync",
                                                      notify ? "on" : "off") < 0) {
            // Error
            return;
        }
        bubbleNotifycation_ = notify;
    }
}

void SettingsManager::setAutoStart(bool autoStart)
{
    qDebug("%s", __func__);
    if (autoStart_ != autoStart) {
        if (set_seafile_auto_start (autoStart) >= 0)
            autoStart_ = autoStart;
    }
}

void SettingsManager::setEncryptTransfer(bool encrypted)
{
    qDebug("%s", __func__);
    if (transferEncrypted_ != encrypted) {
        if (seafApplet->rpcClient()->ccnetSetConfig("encrypt_channel",
                                                    encrypted ? "on" : "off") < 0) {
            // Error
            return;
        }
        transferEncrypted_ = encrypted;
    }
}

void SettingsManager::setMaxDownloadRatio(unsigned int ratio)
{
    qDebug("%s:%d", __func__, ratio);
    if (maxDownloadRatio_ != ratio) {
        if (seafApplet->rpcClient()->seafileSetConfigInt("download_limit",
                                                         ratio << 10) < 0) {
            // Error
            return;
        }
        maxDownloadRatio_ = ratio;
    }
}

void SettingsManager::setMaxUploadRatio(unsigned int ratio)
{
    qDebug("%s:%d", __func__, ratio);
    if (maxUploadRatio_ != ratio) {
        if (seafApplet->rpcClient()->seafileSetConfigInt("upload_limit",
                                                         ratio << 10) < 0) {
            // Error
            return;
        }
        maxUploadRatio_ = ratio;
    }
}

bool SettingsManager::hideMainWindowWhenStarted()
{
    QSettings settings;
    bool hide;

    settings.beginGroup(kBehaviorGroup);
    hide = settings.value(kHideMainWindowWhenStarted, false).toBool();
    settings.endGroup();

    return hide;
}

void SettingsManager::setHideMainWindowWhenStarted(bool hide)
{
    QSettings settings;

    settings.beginGroup(kBehaviorGroup);
    settings.setValue(kHideMainWindowWhenStarted, hide);
    settings.endGroup();
}
