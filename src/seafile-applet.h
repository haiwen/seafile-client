#ifndef SEAFILE_CLIENT_APPLET_H
#define SEAFILE_CLIENT_APPLET_H

#include <QObject>

class AccountManager;
class RpcClient;

class SeafileApplet : QObject {
    Q_OBJECT

public:
    SeafileApplet();

    void start();

    AccountManager *account_mgr;
    RpcClient *rpc_client;

private:
    Q_DISABLE_COPY(SeafileApplet)
};

extern SeafileApplet *seafApplet;

#endif // SEAFILE_CLIENT_APPLET_H
