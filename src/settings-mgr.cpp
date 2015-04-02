#include <QSettings>
#include <QHostInfo>
#include <QNetworkProxy>
#include "utils/utils.h"
#include "utils/utils-mac.h"
#include "seafile-applet.h"
#include "ui/tray-icon.h"
#include "settings-mgr.h"
#include "rpc/rpc-client.h"
#include "utils/utils.h"
#include "network-mgr.h"
#include "ui/main-window.h"

#if defined(Q_OS_WIN32)
#include "utils/registry.h"
#endif

#ifdef HAVE_FINDER_SYNC_SUPPORT
#include "finder-sync/finder-sync.h"
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
#ifdef HAVE_FINDER_SYNC_SUPPORT
const char *kFinderSync = "finderSync";
#endif // HAVE_FINDER_SYNC_SUPPORT
#ifdef HAVE_SHIBBOLETH_SUPPORT
const char *kLastShibUrl = "lastShiburl";
#endif // HAVE_SHIBBOLETH_SUPPORT

} // namespace


SettingsManager::SettingsManager()
    : auto_sync_(true),
      bubbleNotifycation_(true),
      autoStart_(false),
      transferEncrypted_(true),
      allow_invalid_worktree_(false),
      allow_repo_not_found_on_server_(false),
      sync_extra_temp_file_(false),
      maxDownloadRatio_(0),
      maxUploadRatio_(0),
      http_sync_enabled_(false),
      verify_http_sync_cert_disabled_(false),
      use_proxy_type_(NoneProxy),
      proxy_port_(0)
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

    if (seafApplet->rpcClient()->seafileGetConfig("enable_http_sync", &str) >= 0)
        http_sync_enabled_ = (str == "true") ? true : false;

    if (seafApplet->rpcClient()->seafileGetConfig("disable_verify_certificate", &str) >= 0)
        verify_http_sync_cert_disabled_ = (str == "true") ? true : false;

    // reading proxy settings
    do {
        if (seafApplet->rpcClient()->seafileGetConfig("use_proxy", &str) < 0 ) {
            setProxy(NoneProxy);
            break;
        }
        if (str == "false") {
            setProxy(NoneProxy);
            break;
        }
        QString proxy_host;
        int proxy_port;
        QString proxy_username;
        QString proxy_password;
        if (seafApplet->rpcClient()->seafileGetConfig("proxy_addr", &proxy_host) < 0) {
            setProxy(NoneProxy);
            break;
        }
        if (seafApplet->rpcClient()->seafileGetConfigInt("proxy_port", &proxy_port) < 0) {
            setProxy(NoneProxy);
            break;
        }
        if (seafApplet->rpcClient()->seafileGetConfig("proxy_type", &str) < 0) {
            setProxy(NoneProxy);
            break;
        }
        if (str == "http") {
            if (seafApplet->rpcClient()->seafileGetConfig("proxy_username", &proxy_username) < 0) {
                setProxy(NoneProxy);
                break;
            }
            if (seafApplet->rpcClient()->seafileGetConfig("proxy_password", &proxy_password) < 0) {
                setProxy(NoneProxy);
                break;
            }
            setProxy(HttpProxy, proxy_host, proxy_port, proxy_username, proxy_password);
        } else if (str == "socks") {
            setProxy(SocksProxy, proxy_host, proxy_port);
        } else {
            if (!str.isEmpty())
                qWarning("Unsupported proxy_type %s", str.toUtf8().data());
            setProxy(NoneProxy);
        }
    } while(0);


    autoStart_ = get_seafile_auto_start();

#ifdef HAVE_FINDER_SYNC_SUPPORT
    // try to do a reinstall, or we may use findersync somewhere else
    // this action won't stop findersync if running already
    FinderSyncExtensionHelper::reinstall();

    // try to sync finder sync extension settings with the actual settings
    // i.e. enabling the finder sync if the setting is true
    setFinderSyncExtension(getFinderSyncExtension());
#endif // HAVE_FINDER_SYNC_SUPPORT


#ifdef Q_OS_WIN32
    RegElement reg(HKEY_CURRENT_USER, "SOFTWARE\\Seafile", "ShellExtDisabled", "");
    shell_ext_enabled_ = !reg.exists();
#endif
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
#ifdef Q_OS_MAC
    // for UIElement application, the main window might sink
    // under many applications
    // this will force it to stand before all
    utils::mac::orderFrontRegardless(seafApplet->mainWindow()->winId());
#endif
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

#if defined(Q_OS_WIN32)
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
void SettingsManager::getProxy(QNetworkProxy *proxy) {
    proxy->setType(use_proxy_type_ == HttpProxy ? QNetworkProxy::HttpProxy : QNetworkProxy::Socks5Proxy);
    proxy->setHostName(proxy_host_);
    proxy->setPort(proxy_port_);
    if (use_proxy_type_ == HttpProxy && !proxy_username_.isEmpty() && !proxy_password_.isEmpty()) {
        proxy->setUser(proxy_username_);
        proxy->setPassword(proxy_password_);
    }
}

void SettingsManager::setProxy(SettingsManager::ProxyType proxy_type, const QString &proxy_host, int proxy_port, const QString &proxy_username, const QString &proxy_password) {
    // NoneProxy ?
    if (proxy_type == NoneProxy) {
        if (seafApplet->rpcClient()->seafileSetConfig("use_proxy", "false") < 0)
            return;
        use_proxy_type_ = proxy_type;
        QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy);
        return;
    }
    // Use the same Https/Socks Proxy?
    if (use_proxy_type_ == proxy_type && proxy_host_ == proxy_host && proxy_port_ == proxy_port) {
        if (proxy_type == SocksProxy)
            return;
        if (proxy_type == HttpProxy && proxy_username_ == proxy_username && proxy_password_ == proxy_username)
            return;
    }
    // invalid settings
    if (proxy_type != SocksProxy && proxy_type != HttpProxy) {
        return;
    }
    // invalid settings
    if (proxy_host.isEmpty()) {
        return;
    }

    // Otherwise, write settings
    if (seafApplet->rpcClient()->seafileSetConfig("use_proxy", "true") < 0)
        return;
    if (seafApplet->rpcClient()->seafileSetConfig("proxy_type", proxy_type == HttpProxy ? "http" : "socks") < 0)
        return;
    if (seafApplet->rpcClient()->seafileSetConfig("proxy_addr", proxy_host.toUtf8().data()) < 0)
        return;
    if (seafApplet->rpcClient()->seafileSetConfig("proxy_port", QVariant(proxy_port).toString().toUtf8().data()) < 0)
        return;
    if (proxy_type == HttpProxy) {
        if (seafApplet->rpcClient()->seafileSetConfig("proxy_username", proxy_username.toUtf8().data()) < 0)
            return;
        if (seafApplet->rpcClient()->seafileSetConfig("proxy_password", proxy_password.toUtf8().data()) < 0)
            return;
    }
    // skip invalid port
    if (proxy_type == HttpProxy && proxy_port == 0)
        proxy_port = 80;
    if (proxy_port == 0)
        return;

    // save settings
    use_proxy_type_ = proxy_type;
    proxy_host_ = proxy_host;
    proxy_port_ = proxy_port;
    if (proxy_type == HttpProxy) {
        proxy_username_ = proxy_username;
        proxy_password_ = proxy_password;
    }

    // apply settings
    QNetworkProxy proxy;
    getProxy(&proxy);
    NetworkManager::instance()->applyProxy(proxy);
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

void SettingsManager::setHttpSyncEnabled(bool enabled)
{
    if (http_sync_enabled_ != enabled) {
        if (seafApplet->rpcClient()->seafileSetConfig("enable_http_sync",
                                                      enabled ? "true" : "false") < 0) {
            // Error
            return;
        }
        http_sync_enabled_ = enabled;
    }
}

void SettingsManager::setHttpSyncCertVerifyDisabled(bool disabled)
{
    if (verify_http_sync_cert_disabled_ != disabled) {
        if (seafApplet->rpcClient()->seafileSetConfig("disable_verify_certificate",
                                                      disabled ? "true" : "false") < 0) {
            // Error
            return;
        }
        verify_http_sync_cert_disabled_ = disabled;
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

#ifdef HAVE_SHIBBOLETH_SUPPORT
QString SettingsManager::getLastShibUrl()
{
    QSettings settings;
    QString url;

    settings.beginGroup(kSettingsGroup);
    url = settings.value(kLastShibUrl, "").toString();
    settings.endGroup();

    return url;
}

void SettingsManager::setLastShibUrl(const QString& url)
{
    QSettings settings;
    settings.beginGroup(kSettingsGroup);
    settings.setValue(kLastShibUrl, url);
    settings.endGroup();
}
#endif // HAVE_SHIBBOLETH_SUPPORT

#ifdef HAVE_FINDER_SYNC_SUPPORT
bool SettingsManager::getFinderSyncExtension() const
{
    QSettings settings;
    bool enabled;

    settings.beginGroup(kSettingsGroup);
    enabled = settings.value(kFinderSync, true).toBool();
    settings.endGroup();

    return enabled;
}
bool SettingsManager::getFinderSyncExtensionAvailable() const
{
    return FinderSyncExtensionHelper::isInstalled();
}
void SettingsManager::setFinderSyncExtension(bool enabled)
{
    QSettings settings;

    settings.beginGroup(kSettingsGroup);
    settings.setValue(kFinderSync, enabled);
    settings.endGroup();

    // if setting operation fails
    if (!getFinderSyncExtensionAvailable()) {
        qWarning("Unable to find FinderSync Extension");
    } else if (enabled != FinderSyncExtensionHelper::isEnabled() &&
        !FinderSyncExtensionHelper::setEnable(enabled)) {
        qWarning("Unable to enable FinderSync Extension");
    }
}
#endif // HAVE_FINDER_SYNC_SUPPORT

#ifdef Q_OS_WIN32
void SettingsManager::setShellExtensionEnabled(bool enabled)
{
    shell_ext_enabled_ = enabled;

    RegElement reg1(HKEY_CURRENT_USER, "SOFTWARE\\Seafile", "", "");
    RegElement reg2(HKEY_CURRENT_USER, "SOFTWARE\\Seafile", "ShellExtDisabled", "1");
    if (enabled) {
        reg2.remove();
    } else {
        reg1.add();
        reg2.add();
    }
}
#endif  // Q_OS_WIN32
