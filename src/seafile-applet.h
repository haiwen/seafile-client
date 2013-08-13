#ifndef SEAFILE_CLIENT_APPLET_H
#define SEAFILE_CLIENT_APPLET_H

#include <QObject>

class Configurator;
class DaemonManager;
class RpcClient;
class AccountManager;
class MainWindow;

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
    RpcClient *rpcClient() { return rpc_client_; }
    DaemonManager *daemonManager() { return daemon_mgr_; }
    Configurator *configurator() { return configurator_; }
    MainWindow *mainWindow() { return main_win_; }

private slots:
    void onCcnetDaemonConnected();
    void onCcnetDaemonDisconnected();

private:
    Q_DISABLE_COPY(SeafileApplet)

    Configurator *configurator_;
    DaemonManager *daemon_mgr_;
    MainWindow* main_win_;
    RpcClient *rpc_client_;
    AccountManager *account_mgr_;
};

/**
 * The global SeafileApplet object
 */
extern SeafileApplet *seafApplet;


#endif // SEAFILE_CLIENT_APPLET_H
