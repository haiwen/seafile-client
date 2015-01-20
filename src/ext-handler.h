#ifndef SEAFILE_CLIENT_EXT_HANLDER_H
#define SEAFILE_CLIENT_EXT_HANLDER_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QList>

#include <windows.h>

#include "utils/singleton.h"
#include "rpc/local-repo.h"
#include "account.h"

class SeafileRpcClient;
class ExtHandlerThread;

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
    ExtHandlerThread *handler_thread_;

    Account findAccountByRepo(const QString& repo_id);
};

class ExtHandlerThread : public QThread {
    Q_OBJECT
public:
    void run();

signals:
    void generateShareLink(const QString& repo_id,
                           const QString& path_in_repo,
                           bool is_file);

private:
    void serveOneConnection(HANDLE hPipe);
    QList<LocalRepo> listLocalRepos();
    bool readRequest(HANDLE pipe, QStringList *args);
    bool sendRespose(HANDLE pipe, const QString& resp);

    void handleGenShareLink(const QStringList& args);
    QString handleListRepos(const QStringList& args);

    SeafileRpcClient *rpc_;

    QMutex mutex_;
};

#endif // SEAFILE_CLIENT_EXT_HANLDER_H
