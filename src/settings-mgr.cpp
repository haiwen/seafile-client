#include <QSettings>
#include <QHostInfo>
#include "seafile-applet.h"
#include "ui/tray-icon.h"
#include "settings-mgr.h"
#include "rpc/rpc-client.h"
#include "utils/utils.h"

#if defined(Q_OS_WIN)
#include "utils/registry.h"
#endif

namespace {

const char *kHideMainWindowWhenStarted = "hideMainWindowWhenStarted";
const char *kHideDockIcon = "hideDockIcon";
const char *kCheckLatestVersion = "checkLatestVersion";
const char *kBehaviorGroup = "Behavior";

//const char *kDefaultLibraryAlreadySetup = "defaultLibraryAlreadySetup";
//const char *kStatusGroup = "Status";

const char *kSettingsGroup = "Settings";
const char *kComputerName = "computerName";

} // namespace


SettingsManager::SettingsManager()
    : auto_sync_(true),
      bubbleNotifycation_(true),
      autoStart_(false),
      transferEncrypted_(true),
      allow_invalid_worktree_(false),
      allow_repo_not_found_on_server_(false),
      maxDownloadRatio_(0),
      maxUploadRatio_(0),
      sync_extra_temp_file_(false)
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

    if (seafApplet->rpcClient()->seafileGetConfig("allow_invalid_worktree", &str) >= 0)
        allow_invalid_worktree_ = (str == "true") ? true : false;

    if (seafApplet->rpcClient()->seafileGetConfig("sync_extra_temp_file", &str) >= 0)
        sync_extra_temp_file_ = (str == "true") ? true : false;

    if (seafApplet->rpcClient()->seafileGetConfig("allow_repo_not_found_on_server", &str) >= 0)
        allow_repo_not_found_on_server_ = (str == "true") ? true : false;

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
    if (autoStart_ != autoStart) {
        if (set_seafile_auto_start (autoStart) >= 0)
            autoStart_ = autoStart;
    }
}

void SettingsManager::setEncryptTransfer(bool encrypted)
{
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
    if (maxDownloadRatio_ != ratio) {
        if (seafApplet->rpcClient()->setDownloadRateLimit(ratio << 10) < 0) {
            // Error
            return;
        }
        maxDownloadRatio_ = ratio;
    }
}

void SettingsManager::setMaxUploadRatio(unsigned int ratio)
{
    if (maxUploadRatio_ != ratio) {
        if (seafApplet->rpcClient()->setUploadRateLimit(ratio << 10) < 0) {
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

bool SettingsManager::hideDockIcon()
{
    QSettings settings;
    bool hide;

    settings.beginGroup(kBehaviorGroup);
    hide = settings.value(kHideDockIcon, false).toBool();
    settings.endGroup();
    return hide;
}

void SettingsManager::setHideDockIcon(bool hide)
{
    QSettings settings;

    settings.beginGroup(kBehaviorGroup);
    settings.setValue(kHideDockIcon, hide);
    settings.endGroup();

    set_seafile_dock_icon_style(hide);
}

// void SettingsManager::setDefaultLibraryAlreadySetup()
// {
//     QSettings settings;

//     settings.beginGroup(kStatusGroup);
//     settings.setValue(kDefaultLibraryAlreadySetup, true);
//     settings.endGroup();
// }


// bool SettingsManager::defaultLibraryAlreadySetup()
// {
//     QSettings settings;
//     bool done;

//     settings.beginGroup(kStatusGroup);
//     done = settings.value(kDefaultLibraryAlreadySetup, false).toBool();
//     settings.endGroup();

//     return done;
// }

void SettingsManager::removeAllSettings()
{
    QSettings settings;
    settings.clear();

#if defined(Q_OS_WIN)
    RegElement::removeRegKey(HKEY_CURRENT_USER, "SOFTWARE", getBrand());
#endif
}

void SettingsManager::setCheckLatestVersionEnabled(bool enabled)
{
    QSettings settings;

    settings.beginGroup(kBehaviorGroup);
    settings.setValue(kCheckLatestVersion, enabled);
    settings.endGroup();
}

bool SettingsManager::isCheckLatestVersionEnabled()
{
    QString brand = getBrand();

    if (brand != "Seafile") {
        return false;
    }

    QSettings settings;
    bool enabled;

    settings.beginGroup(kBehaviorGroup);
    enabled = settings.value(kCheckLatestVersion, true).toBool();
    settings.endGroup();

    return enabled;
}

void SettingsManager::setAllowInvalidWorktree(bool val)
{
    if (allow_invalid_worktree_ != val) {
        if (seafApplet->rpcClient()->seafileSetConfig("allow_invalid_worktree",
                                                      val ? "true" : "false") < 0) {
            // Error
            return;
        }
        allow_invalid_worktree_ = val;
    }
}

void SettingsManager::setSyncExtraTempFile(bool sync)
{
    if (sync_extra_temp_file_ != sync) {
        if (seafApplet->rpcClient()->seafileSetConfig(
                "sync_extra_temp_file",
                sync ? "true" : "false") < 0) {
            // Error
            return;
        }
        sync_extra_temp_file_ = sync;
    }
}

void SettingsManager::setAllowRepoNotFoundOnServer(bool val)
{
    if (allow_repo_not_found_on_server_ != val) {
        if (seafApplet->rpcClient()->seafileSetConfig("allow_repo_not_found_on_server",
                                                      val ? "true" : "false") < 0) {
            // Error
            return;
        }
        allow_repo_not_found_on_server_ = val;
    }
}

QString SettingsManager::getComputerName()
{
    QSettings settings;
    QString name;

    QString default_computer_Name = QHostInfo::localHostName();

    settings.beginGroup(kSettingsGroup);
    name = settings.value(kComputerName, default_computer_Name).toString();
    settings.endGroup();

    return name;
}

void SettingsManager::setComputerName(const QString& computerName)
{
    QSettings settings;
    settings.beginGroup(kSettingsGroup);
    settings.setValue(kComputerName, computerName);
    settings.endGroup();
}
