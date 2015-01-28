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

/**
 * Handles commands from seafile shell extension
 */
class SeafileExtensionHandler : public QObject {
    SINGLETON_DEFINE(SeafileExtensionHandler)
    Q_OBJECT
public:
    SeafileExtensionHandler();
    void start();

private slots:
    void onShareLinkGenerated(const QString& link);
    void generateShareLink(const QString& repo_id,
                           const QString& path_in_repo,
                           bool is_file);

private:
    ExtConnectionListenerThread *listener_thread_;

    Account findAccountByRepo(const QString& repo_id);

    QHash<QString, Account> accounts_cache_;
};

/**
 * Creates the named pipe and listen for incoming connections in a separate
 * thread.
 *
 * When a connection is accepted, create a new ExtCommandsHandler thread to
 * servce it.
 */
class ExtConnectionListenerThread : public QThread {
    Q_OBJECT
public:
    void run();

signals:
    void generateShareLink(const QString& repo_id,
                           const QString& path_in_repo,
                           bool is_file);

private:
    void servePipeInNewThread(HANDLE pipe);

};

/**
 * Serves one extension connection.
 *
 * It's a endless loop of "read request" -> "handle request" -> "send response".
 */
class ExtCommandsHandler: public QThread {
    Q_OBJECT
public:
    ExtCommandsHandler(HANDLE pipe);
    void run();

signals:
    void generateShareLink(const QString& repo_id,
                           const QString& path_in_repo,
                           bool is_file);

private:
    HANDLE pipe_;

    QList<LocalRepo> listLocalRepos();
    bool readRequest(QStringList *args);
    bool sendResponse(const QString& resp);

    void handleGenShareLink(const QStringList& args);
    QString handleListRepos(const QStringList& args);

    SeafileRpcClient *rpc_;

    static QMutex rpc_mutex_;
};

#endif // SEAFILE_CLIENT_EXT_HANLDER_H
