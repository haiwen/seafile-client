#ifndef SEAFILE_CLIENT_FINDER_SYNC_HOST_H_
#define SEAFILE_CLIENT_FINDER_SYNC_HOST_H_
#include <QObject>
#include <QString>
#include <cstdint>
#include <vector>
#include "utils/stl.h"

const int kWatchDirMax = 100;
const int kPathMaxSize = 1024;

class Account;
class SeafileRpcClient;
class FinderSyncHost : public QObject {
    Q_OBJECT
public:
    FinderSyncHost();
    ~FinderSyncHost();
    // called from another thread
    utils::BufferArray getWatchSet(size_t header_size, int max_size = -1);
    // called from another thread
    uint32_t getFileStatus(const char* repo_id, const char* path);
private slots:
    void onDaemonRestarted();
    void updateWatchSet();
    void doLockFile(const QString &path, bool lock);
    void doShareLink(const QString &path);
    void doInternalLink(const QString &path);
    void onShareLinkGenerated(const QString& link);
    void onLockFileSuccess();
    void doShowFileHistory(const QString& path);
private:
    bool lookUpFileInformation(const QString &path, QString *repo_id, Account *account, QString *path_in_repo);
    SeafileRpcClient *rpc_client_;
};

#endif // SEAFILE_CLIENT_FINDER_SYNC_HOST_H_
