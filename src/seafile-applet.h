#ifndef SEAFILE_CLIENT_APPLET_H
#define SEAFILE_CLIENT_APPLET_H

#include <QObject>

class AccountManager;
class RpcClient;
class Configurator;
class MainWindow;

class SeafileApplet : QObject {
    Q_OBJECT

public:
    SeafileApplet();

    void start();

    // normal exit
    void exit(int code);

    // Show error in a messagebox and exit
    void errorAndExit(const QString& error);

    AccountManager *account_mgr;
    RpcClient *rpc_client;
    Configurator *configurator;
    MainWindow* main_win;

private:
    Q_DISABLE_COPY(SeafileApplet)
};

// The global SeafileApplet object
extern SeafileApplet *seafApplet;

#endif // SEAFILE_CLIENT_APPLET_H
