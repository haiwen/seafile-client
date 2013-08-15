#ifndef SEAFILE_CLIENT_RPC_CLIENT_H
#define SEAFILE_CLIENT_RPC_CLIENT_H

#include <QObject>

extern "C" {

struct _CcnetClient;
// Can't forward-declare type SearpcClient here because it is an anonymous typedef struct
#include <searpc-client.h>

}

class QSocketNotifier;

class SeafileRpcClient : QObject {
    Q_OBJECT

public:
    SeafileRpcClient();
    void start();
    void connectDaemon();

    SearpcClient* seafRpcClient() { return seafile_rpc_client_; }
    SearpcClient* ccnetRpcClient() { return ccnet_rpc_client_; }

private slots:
    void readConnfd();

private:
    Q_DISABLE_COPY(SeafileRpcClient)

    _CcnetClient *async_client_;
    SearpcClient *seafile_rpc_client_;
    SearpcClient *ccnet_rpc_client_;
    
    QSocketNotifier *socket_notifier_;
};

#endif
