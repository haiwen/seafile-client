#ifndef SEAFILE_CLIENT_APPLET_H
#define SEAFILE_CLIENT_APPLET_H

#include <QObject>

class Configurator;
class DaemonManager;
class SeafileRpcClient;
class AccountManager;
class MainWindow;
class MessageListener;
class SeafileTrayIcon;
class SettingsManager;
class SettingsDialog;

/**
 * The central class of seafile-client
 */
class SeafileApplet : QObject {
    Q_OBJECT

public:
    SeafileApplet();

    void start();

    // normal exit
    void exit(int code);

    // Show error in a messagebox and exit
    void errorAndExit(const QString& error);

    // accessors
    AccountManager *accountManager() { return account_mgr_; }

    SeafileRpcClient *rpcClient() { return rpc_client_; }

    DaemonManager *daemonManager() { return daemon_mgr_; }

    Configurator *configurator() { return configurator_; }

    MainWindow *mainWindow() { return main_win_; }

    SeafileTrayIcon *trayIcon() { return tray_icon_; }

    SettingsDialog *settingsDialog() { return settings_dialog_; }

    SettingsManager *settingsManager() { return settings_mgr_; }

    bool inExit() { return in_exit_; }

private slots:
    void onDaemonStarted();

private:
    Q_DISABLE_COPY(SeafileApplet)

    Configurator *configurator_;

    DaemonManager *daemon_mgr_;

    MainWindow* main_win_;

    SeafileRpcClient *rpc_client_;

    AccountManager *account_mgr_;

    MessageListener *message_listener_;

    SeafileTrayIcon *tray_icon_;

    SettingsDialog *settings_dialog_;

    SettingsManager *settings_mgr_;

    bool in_exit_;
};

/**
 * The global SeafileApplet object
 */
extern SeafileApplet *seafApplet;


#endif // SEAFILE_CLIENT_APPLET_H
