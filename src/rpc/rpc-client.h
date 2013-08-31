#ifndef SEAFILE_CLIENT_RPC_CLIENT_H
#define SEAFILE_CLIENT_RPC_CLIENT_H

#include <QObject>
#include <vector>

extern "C" {

struct _CcnetClient;
// Can't forward-declare type SearpcClient here because it is an anonymous typedef struct
#include <searpc-client.h>

}

class LocalRepo;
class QSocketNotifier;

class SeafileRpcClient : public QObject {
    Q_OBJECT

public:
    SeafileRpcClient();
    void start();
    void connectDaemon();

    SearpcClient* seafRpcClient() { return seafile_rpc_client_; }
    SearpcClient* ccnetRpcClient() { return ccnet_rpc_client_; }

    void listLocalRepos();
    void setAutoSync(bool autoSync);
    void downloadRepo(const QString &id, const QString &relayId,
                      const QString &name, const QString &wt,
                      const QString &token, const QString &passwd,
                      const QString &magic, const QString &peerAddr,
                      const QString &port, const QString &email);
    void cloneRepo(const QString &id, const QString &relayId,
                   const QString &name, const QString &wt,
                   const QString &token, const QString &passwd,
                   const QString &magic, const QString &peerAddr,
                   const QString &port, const QString &email);

signals:
    void listLocalReposSignal(const std::vector<LocalRepo> &repos, bool result);
    void setAutoSyncSignal(bool autoSync, bool result);
    void downloadRepoSignal(QString &repoId, bool result);
    void cloneRepoSignal(QString &repoId, bool result);

private:
    static void listLocalReposCB(void *result, void *data, GError *error);
    static void setAutoSyncCB(void *result, void *data, GError *error);
    static void setNotAutoSyncCB(void *result, void *data, GError *error);
    static void downloadRepoCB(void *result, void *data, GError *error);
    static void cloneRepoCB(void *result, void *data, GError *error);


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
