#ifndef SEAFILE_CLIENT_SETTINGS_MANAGER_H
#define SEAFILE_CLIENT_SETTINGS_MANAGER_H

#include <QObject>

/**
 * Settings Manager handles seafile client user settings & preferences
 */
class QNetworkProxy;
class SettingsManager : public QObject {
    Q_OBJECT

public:
    enum ProxyType {
        NoneProxy = 0,
        HttpProxy = 1,
        SocksProxy = 2
    };
    SettingsManager();

    void loadSettings();
    void setAutoSync(bool);

    bool autoSync() { return auto_sync_; }

    bool notify() { return bubbleNotifycation_; }
    bool autoStart() { return autoStart_; }
    bool encryptTransfer() { return transferEncrypted_; }
    unsigned int maxDownloadRatio() { return maxDownloadRatio_; }
    unsigned int maxUploadRatio() { return maxUploadRatio_; }
    bool allowInvalidWorktree() { return allow_invalid_worktree_; }
    bool syncExtraTempFile() { return sync_extra_temp_file_; }
    void getProxy(QNetworkProxy *proxy);
    void getProxy(ProxyType &proxy_type, QString &proxy_host, int &proxy_port, QString &proxy_username, QString &proxy_password) {
        proxy_type = use_proxy_type_;
        proxy_host = proxy_host_;
        proxy_port = proxy_port_;
        proxy_username = proxy_username_;
        proxy_password = proxy_password_;
    }

    void setNotify(bool notify);
    void setAutoStart(bool autoStart);
    void setEncryptTransfer(bool encrypted);
    void setMaxDownloadRatio(unsigned int ratio);
    void setMaxUploadRatio(unsigned int ratio);
    void setAllowInvalidWorktree(bool val);
    void setSyncExtraTempFile(bool sync);
    void setProxy(ProxyType proxy_type, const QString &proxy_host = QString(), int proxy_port = 0, const QString &proxy_username = QString(), const QString &proxy_password = QString());

    bool hideMainWindowWhenStarted();
    void setHideMainWindowWhenStarted(bool hide);

    bool hideDockIcon();
    void setHideDockIcon(bool hide);

    void setCheckLatestVersionEnabled(bool enabled);
    bool isCheckLatestVersionEnabled();
    // bool defaultLibraryAlreadySetup();
    // void setDefaultLibraryAlreadySetup();

    void setAllowRepoNotFoundOnServer(bool enabled);
    bool allowRepoNotFoundOnServer() const { return allow_repo_not_found_on_server_; };

    void setHttpSyncEnabled(bool enabled);
    bool httpSyncEnabled() const { return http_sync_enabled_; };

    void setHttpSyncCertVerifyDisabled(bool disabled);
    bool httpSyncCertVerifyDisabled() const { return verify_http_sync_cert_disabled_; };

    QString getComputerName();
    void setComputerName(const QString& computerName);

#ifdef HAVE_SHIBBOLETH_SUPPORT
    QString getLastShibUrl();
    void setLastShibUrl(const QString& url);
#endif // HAVE_SHIBBOLETH_SUPPORT

#ifdef HAVE_FINDER_SYNC_SUPPORT
    bool getFinderSyncExtension() const;
    bool getFinderSyncExtensionAvailable() const;
    void setFinderSyncExtension(bool enabled);
#endif // HAVE_FINDER_SYNC_SUPPORT

#ifdef Q_OS_WIN32
    void setShellExtensionEnabled(bool enabled);
    bool shellExtensionEnabled() const { return shell_ext_enabled_; }
#endif // HAVE_FINDER_SYNC_SUPPORT

public:

    // Remove all settings from system when uninstall
    static void removeAllSettings();

private:
    Q_DISABLE_COPY(SettingsManager)

    bool auto_sync_;
    bool bubbleNotifycation_;
    bool autoStart_;
    bool transferEncrypted_;
    bool allow_invalid_worktree_;
    bool allow_repo_not_found_on_server_;
    bool sync_extra_temp_file_;
    unsigned int maxDownloadRatio_;
    unsigned int maxUploadRatio_;
    bool http_sync_enabled_;
    bool verify_http_sync_cert_disabled_;
    bool shell_ext_enabled_;

    // proxy settings
    ProxyType use_proxy_type_;
    QString proxy_host_;
    int proxy_port_;
    QString proxy_username_;
    QString proxy_password_;
};

#endif // SEAFILE_CLIENT_SETTINGS_MANAGER_H
