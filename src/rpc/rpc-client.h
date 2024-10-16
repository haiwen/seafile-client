#ifndef SEAFILE_CLIENT_RPC_CLIENT_H
#define SEAFILE_CLIENT_RPC_CLIENT_H

#include <vector>

#include <QObject>
#include <QMutex>

extern "C" {

struct _GList;
// Can't forward-declare type SearpcClient here because it is an anonymous typedef struct
#include <searpc-client.h>

}

// Here we can't forward-declare type json_t because it is an anonymous typedef
// struct, and unlike libsearpc we have no way to rewrite its definition to give
// it a name.
#include <jansson.h>

class LocalRepo;
class CloneTask;
class SyncError;
class CommitDetails;

class SeafileRpcClient : public QObject {
    Q_OBJECT

public:
    SeafileRpcClient();
    ~SeafileRpcClient();
    bool tryConnectDaemon() { return connectDaemon(false); }
    bool connectDaemon(bool exit_on_error = true);
    bool isConnected() const { return connected_; }

    int listLocalRepos(std::vector<LocalRepo> *repos);
    int getLocalRepo(const QString& repo_id, LocalRepo *repo);
    int setAutoSync(const bool autoSync);
    int downloadRepo(const QString& id,
                     int repo_version, const QString& name,
                     const QString& wt, const QString& token,
                     const QString& passwd, const QString& magic,
                     const QString& email, const QString& random_key,
                     int enc_version, const QString& more_info,
                     QString *error);

    int cloneRepo(const QString& id,
                  int repo_version, const QString& name,
                  const QString& wt, const QString& token,
                  const QString& passwd, const QString& magic,
                  const QString& email, const QString& random_key,
                  int enc_version, const QString& more_info,
                  QString *error);

    int seafileGetConfig(const QString& key, QString *value);
    int seafileGetConfigInt(const QString& key, int *value);
    int seafileSetConfig(const QString& key, const QString& value);
    int seafileSetConfigInt(const QString& key, int value);

    void getSyncStatus(LocalRepo &repo);
    int getCloneTasks(std::vector<CloneTask> *tasks);
    int getCloneTasksCount(int *count);

    int cancelCloneTask(const QString& repo_id, QString *error);
    int removeCloneTask(const QString& repo_id, QString *error);

    int unsyncReposByAccount(const QUrl& server_url, const QString& email, QString *err);
    int removeSyncTokensByAccount(const QUrl& server_url, const QString& email, QString *error);
    int getUploadRate(int *rate);
    int getDownloadRate(int *rate);

    int getRepoTransferInfo(const QString& repo_id, int *rate, int *percent, QString* rt_state=nullptr);

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

    int updateReposServerHost(const QUrl& old_server_url,
                              const QString& new_server_url,
                              QString *err);

    int getRepoProperty(const QString& repo_id,
                        const QString& name,
                        QString *value);

    int setRepoProperty(const QString& repo_id,
                        const QString& name,
                        const QString& value);

    int setRepoToken(const QString &repo_id,
                     const QString& token);

    int getRepoFileStatus(const QString& repo_id,
                          const QString& path_in_repo,
                          bool isdir,
                          QString *status);

    int markFileLockState(const QString& repo_id,
                          const QString& path_in_repo,
                          bool lock);

    int generateMagicAndRandomKey(int enc_version,
                                  const QString& repo_id,
                                  const QString& passwd,
                                  const QString& pwd_hash_algo,
                                  const QString& pwd_hash_params,
                                  QString *magic,
                                  QString *pwd_hash,
                                  QString *random_key,
                                  QString *salt);

    bool setServerProperty(const QString &url,
                           const QString &key,
                           const QString &value);

    // Read the commit diff from the daemon. Note this rpc may block for a
    // while.
    bool getCommitDiff(const QString& repo_id,
                       const QString& commit_id,
                       const QString& previous_commit_id,
                       CommitDetails *details);

    bool getSyncErrors(std::vector<SyncError> *errors, int offset=0, int limit=10);

    bool getSyncNotification(json_t **ret);

    bool deleteFileAsyncErrorById(int id);

    bool addDelConfirmation(const QString& confirmation_id, bool resync);

private:
    Q_DISABLE_COPY(SeafileRpcClient)

    void getTransferDetail(CloneTask* task);

    enum Direction {
        DOWNLOAD = 0,
        UPLOAD
    };
    int setRateLimit(Direction, int limit);

    bool connected_;

    QMutex threaded_rpc_mutex_;

    SearpcClient *seafile_rpc_client_;
    SearpcClient *seafile_threaded_rpc_client_;
};

#endif
