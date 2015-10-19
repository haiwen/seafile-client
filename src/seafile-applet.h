#ifndef SEAFILE_CLIENT_APPLET_H
#define SEAFILE_CLIENT_APPLET_H

#include <QObject>
#include <QVariant>
#include <QMessageBox>

class Configurator;
class DaemonManager;
class SeafileRpcClient;
class AccountManager;
class MainWindow;
class MessageListener;
class SeafileTrayIcon;
class SettingsManager;
class SettingsDialog;
class CertsManager;


/**
 * The central class of seafile-client
 */
class SeafileApplet : QObject {
    Q_OBJECT

public:
    SeafileApplet();
    ~SeafileApplet();

    void start();

    void refreshQss();

    void messageBox(const QString& msg, QWidget *parent=0);
    void warningBox(const QString& msg, QWidget *parent=0);
    bool yesOrNoBox(const QString& msg, QWidget *parent=0, bool default_val=true);
    bool detailedYesOrNoBox(const QString& msg, const QString& detailed_text, QWidget *parent, bool default_val=true);
    QMessageBox::StandardButton yesNoCancelBox(const QString& msg,
                                               QWidget *parent,
                                               QMessageBox::StandardButton default_btn);
    bool yesOrCancelBox(const QString& msg, QWidget *parent, bool default_ok);

    // Show error in a messagebox and exit
    void errorAndExit(const QString& error);
    void restartApp();

    // Read preconfigure settings
    QVariant readPreconfigureEntry(const QString& key, const QVariant& default_value = QVariant());
    // ExpandedVars String
    QString readPreconfigureExpandedString(const QString& key, const QString& default_value = QString());

    // accessors
    AccountManager *accountManager() { return account_mgr_; }

    SeafileRpcClient *rpcClient() { return rpc_client_; }

    DaemonManager *daemonManager() { return daemon_mgr_; }

    Configurator *configurator() { return configurator_; }

    MainWindow *mainWindow() { return main_win_; }

    SeafileTrayIcon *trayIcon() { return tray_icon_; }

    SettingsDialog *settingsDialog() { return settings_dialog_; }

    SettingsManager *settingsManager() { return settings_mgr_; }

    CertsManager *certsManager() { return certs_mgr_; }

    bool started() { return started_; }
    bool inExit() { return in_exit_; }

private slots:
    void onDaemonStarted();
    void checkInitVDrive();
    void onGetLatestVersionInfoSuccess(const QString&);
    void updateReposPropertyForHttpSync();
    void onAboutToQuit();

private:
    Q_DISABLE_COPY(SeafileApplet)

    void initLog();

    bool loadQss(const QString& path);

    void checkLatestVersionInfo();

    Configurator *configurator_;

    AccountManager *account_mgr_;

    DaemonManager *daemon_mgr_;

    MainWindow* main_win_;

    SeafileRpcClient *rpc_client_;

    MessageListener *message_listener_;

    SeafileTrayIcon *tray_icon_;

    SettingsDialog *settings_dialog_;

    SettingsManager *settings_mgr_;

    CertsManager *certs_mgr_;

    bool started_;

    bool in_exit_;

    QString style_;

    bool is_pro_;
};

/**
 * The global SeafileApplet object
 */
extern SeafileApplet *seafApplet;

#define STR(s)     #s
#define STRINGIZE(x) STR(x)

#endif // SEAFILE_CLIENT_APPLET_H
