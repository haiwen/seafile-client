#ifndef SEAFILE_CLIENT_RPC_REQUESTS_H
#define SEAFILE_CLIENT_RPC_REQUESTS_H

#include <vector>

#include "rpc-request.h"

class LocalRepo;

class ListLocalReposRequest : public SeafileRpcRequest
{
    Q_OBJECT
    Q_DISABLE_COPY(ListLocalReposRequest)

public:
    ListLocalReposRequest();
    void send();
    void onRpcFinished(void *result);

signals:
    void success(const std::vector<LocalRepo>& repos);
};


class SetAutoSyncRequest : public SeafileRpcRequest
{
    Q_OBJECT

public:
    SetAutoSyncRequest(bool auto_sync);
    void send();
    void onRpcFinished(void *result);

signals:
    void success(bool);

private:
    Q_DISABLE_COPY(SetAutoSyncRequest)
    bool auto_sync_;
};

#endif
