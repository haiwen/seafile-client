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
    int downloadRepo(const QString& id,
                     int repo_version, const QString& relayId,
                     const QString& name, const QString& wt,
                     const QString& token, const QString& passwd,
                     const QString& magic, const QString& peerAddr,
                     const QString& port, const QString& email,
                     const QString& random_key, int enc_version,
                     const QString& more_info,
                     QString *error);

    int cloneRepo(const QString& id,
                  int repo_version, const QString& relayId,
                  const QString& name, const QString& wt,
                  const QString& token, const QString& passwd,
                  const QString& magic, const QString& peerAddr,
                  const QString& port, const QString& email,
                  const QString& random_key, int enc_version,
                  const QString& more_info,
                  QString *error);

    int ccnetGetConfig(const QString& key, QString *value);
    int seafileGetConfig(const QString& key, QString *value);
    int seafileGetConfigInt(const QString& key, int *value);
    int ccnetSetConfig(const QString& key, const QString& value);
    int seafileSetConfig(const QString& key, const QString& value);
    int seafileSetConfigInt(const QString& key, int value);

    void getSyncStatus(LocalRepo &repo);
    int getCloneTasks(std::vector<CloneTask> *tasks);
    int getCloneTasksCount(int *count);

    int cancelCloneTask(const QString& repo_id, QString *error);
    int removeCloneTask(const QString& repo_id, QString *error);

    int unsyncReposByAccount(const QString& server_addr, const QString& email, QString *error);
    int getUploadRate(int *rate);
    int getDownloadRate(int *rate);

    int getRepoTransferInfo(const QString& repo_id, int *rate, int *percent);

    void setRepoAutoSync(const QString& repo_id, bool auto_sync);
    int unsync(const QString& repo_id);

    int setUploadRateLimit(int limit);
    int setDownloadRateLimit(int limit);

    void syncRepoImmediately(const QString& repo_id);

    int checkPathForClone(const QString& path, QString* err_msg);

    // Helper functions
    bool hasLocalRepo(const QString& repo_id);
    int getServers(_GList** servers);

    QString getCcnetPeerId();

    int updateReposServerHost(const QString& old_host,
                              const QString& new_host,
                              QString *err);

private:
    Q_DISABLE_COPY(SeafileRpcClient)

    void getTransferDetail(CloneTask* task);
    void getCheckOutDetail(CloneTask* task);
    int setRateLimit(bool upload, int limit);


    _CcnetClient *sync_client_;
    SearpcClient *seafile_rpc_client_;
    SearpcClient *ccnet_rpc_client_;
};

#endif
