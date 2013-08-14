#ifndef SEAFILE_CLIENT_RPC_CLIENT_H
#define SEAFILE_CLIENT_RPC_CLIENT_H

#include <vector>
#include <QObject>
#include <QString>

extern "C" {

struct _CcnetClient;
// Can't forward-declare type SearpcClient here because it is an anonymous typedef struct
#include <searpc-client.h>

}

#include "local-repo.h"

class RpcClient : QObject {
    Q_OBJECT

public:
    explicit RpcClient(const QString& config_dir);
    void start();
    int listRepos(std::vector<LocalRepo> *result);
    int setAutoSync(bool autoSync);
    bool connected();
    void reconnect();

private:
    Q_DISABLE_COPY(RpcClient)

    QString config_dir_;
    
    _CcnetClient *sync_client_;
    SearpcClient *seafile_rpc_client_;
    SearpcClient *ccnet_rpc_client_;
};

#endif
