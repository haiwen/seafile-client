#ifndef SEAFILE_CLIENT_EXT_HANLDER_H
#define SEAFILE_CLIENT_EXT_HANLDER_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QList>
#include <QHash>

#include <windows.h>

#include "utils/singleton.h"
#include "rpc/local-repo.h"
#include "account.h"

class SeafileRpcClient;
class ExtConnectionListenerThread;
class ApiError;

/**
 * Handles commands from seafile shell extension
 */
class SeafileExtensionHandler : public QObject {
    SINGLETON_DEFINE(SeafileExtensionHandler)
    Q_OBJECT
public:
    SeafileExtensionHandler();
    void start();
    void stop();

private slots:
    void onShareLinkGenerated(const QString& link);
    void onLockFileSuccess();
    void onLockFileFailed(const ApiError& error);
    void generateShareLink(const QString& repo_id,
                           const QString& path_in_repo,
                           bool is_file,
                           bool internal);
    void lockFile(const QString& repo_id,
                  const QString& path_in_repo,
                  bool lock);
    void privateShare(const QString& repo_id,
                      const QString& path_in_repo,
                      bool to_group);
    void openUrlWithAutoLogin(const QUrl& url);

private:
    ExtConnectionListenerThread *listener_thread_;

    bool started_;
};

/**
 * Creates the named pipe and listen for incoming connections in a separate
 * thread.
 *
 * When a connection is accepted, create a new ExtCommandsHandler thread to
 * serve it.
 */
class ExtConnectionListenerThread : public QThread {
    Q_OBJECT
public:
    void run();

signals:
    void generateShareLink(const QString& repo_id,
                           const QString& path_in_repo,
                           bool is_file,
                           bool internal);
    void lockFile(const QString& repo_id,
                  const QString& path_in_repo,
                  bool lock);
    void privateShare(const QString& repo_id,
                      const QString& path_in_repo,
                      bool to_group);
    void openUrlWithAutoLogin(const QUrl& url);

private:
    void servePipeInNewThread(HANDLE pipe);
};

/**
 * Serves one extension connection.
 *
 * It's an endless loop of "read request" -> "handle request" -> "send response".
 */
class ExtCommandsHandler: public QThread {
    Q_OBJECT
public:
    ExtCommandsHandler(HANDLE pipe);
    void run();

signals:
    void generateShareLink(const QString& repo_id,
                           const QString& path_in_repo,
                           bool is_file,
                           bool internal);
    void lockFile(const QString& repo_id,
                  const QString& path_in_repo,
                  bool lock);
    void privateShare(const QString& repo_id,
                      const QString& path_in_repo,
                      bool to_group);
    void openUrlWithAutoLogin(const QUrl& url);

private:
    HANDLE pipe_;

    QList<LocalRepo> listLocalRepos(quint64 ts = 0);
    bool readRequest(QStringList *args);
    bool sendResponse(const QString& resp);

    void handleGenShareLink(const QStringList& args, bool internal);
    QString handleListRepos(const QStringList& args);
    QString handleGetFileStatus(const QStringList& args);
    void handleLockFile(const QStringList& args, bool lock);
    void handlePrivateShare(const QStringList& args, bool to_group);
    void handleShowHistory(const QStringList& args);
};

class ReposInfoCache : public QObject {
    SINGLETON_DEFINE(ReposInfoCache)
    Q_OBJECT
public:
    ReposInfoCache(QObject *parent=0);
    void start();

    QList<LocalRepo> getReposInfo(quint64 timestamp = 0);

    bool getRepoFileStatus(const QString& repo_id,
                           const QString& path_in_repo,
                           bool isdir,
                           QString *status);

private slots:
    void onDaemonRestarted();

private:
    quint64 cache_ts_;
    QList<LocalRepo> cached_info_;

    SeafileRpcClient *rpc_client_;
    QMutex rpc_client_mutex_;
};

#endif // SEAFILE_CLIENT_EXT_HANLDER_H
