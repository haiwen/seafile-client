#ifndef SEAFILE_CLIENT_RPC_SERVER_H
#define SEAFILE_CLIENT_RPC_SERVER_H

#include <QObject>

#include "utils/singleton.h"

struct SeafileAppletRpcServerPriv;

// This is the rpc server componet to accept commands like --stop,
// --remove-user-data, and open seafile:// protocol url, etc.
class SeafileAppletRpcServer : public QObject {
    SINGLETON_DEFINE(SeafileAppletRpcServer)
    Q_OBJECT
public:
    SeafileAppletRpcServer();
    ~SeafileAppletRpcServer();

    void start();

    class Client {
    public:
        virtual bool connect() = 0;
        virtual bool sendPingCommand(QString* resp) = 0;
        virtual bool sendActivateCommand() = 0;
        virtual bool sendExitCommand() = 0;
        virtual bool sendOpenSeafileUrlCommand(const QUrl& url) = 0;
    };

    static Client* getClient();

private:
    SeafileAppletRpcServerPriv *priv_;
};

#endif
