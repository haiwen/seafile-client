#ifndef SEAFILE_CLIENT_RPC_CLIENT_H
#define SEAFILE_CLIENT_RPC_CLIENT_H

#include <QObject>
#include <vector>

extern "C" {

struct _GList;
struct _CcnetClient;
// Can't forward-declare type SearpcClient here because it is an anonymous typedef struct
#include <searpc-client.h>

}

class LocalRepo;
class CloneTask;

class SeafileRpcClient : public QObject {
    Q_OBJECT

public:
    SeafileRpcClient();
    void connectDaemon();

    int listLocalRepos(std::vector<LocalRepo> *repos);
    int getLocalRepo(const QString& repo_id, LocalRepo *repo);
    int setAutoSync(const bool autoSync);
    int downloadRepo(const QString &id, const QString &relayId,
                     const QString &name, const QString &wt,
                     const QString &token, const QString &passwd,
                     const QString &magic, const QString &peerAddr,
                     const QString &port, const QString &email,
                     QString *error);

    int cloneRepo(const QString &id, const QString &relayId,
                  const QString &name, const QString &wt,
                  const QString &token, const QString &passwd,
                  const QString &magic, const QString &peerAddr,
                     const QString &port, const QString &email,
                     QString *error);

    int ccnetGetConfig(const QString &key, QString *value);
    int seafileGetConfig(const QString &key, QString *value);
    int seafileGetConfigInt(const QString &key, int *value);
    int ccnetSetConfig(const QString &key, const QString &value);
    int seafileSetConfig(const QString &key, const QString &value);
    int seafileSetConfigInt(const QString &key, int value);

    void getSyncStatus(LocalRepo &repo);
    int getCloneTasks(std::vector<CloneTask> *tasks);
    int getCloneTasksCount(int *count);

    int cancelCloneTask(const QString& repo_id, QString *error);
    int removeCloneTask(const QString& repo_id, QString *error);

    // Helper functions
    bool hasLocalRepo(const QString& repo_id);
    int getServers(_GList** servers);

private:
    Q_DISABLE_COPY(SeafileRpcClient)

    void getTransferDetail(CloneTask* task);
    void getCheckOutDetail(CloneTask* task);
    

    _CcnetClient *sync_client_;
    SearpcClient *seafile_rpc_client_;
    SearpcClient *ccnet_rpc_client_;
};

#endif
