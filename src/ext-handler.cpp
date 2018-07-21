#include <winsock2.h>
#include <windows.h>
#include <io.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <fcntl.h>
#include <ctype.h>
#include <userenv.h>

#include <string>
#include <QMutexLocker>
#include <QScopedPointer>
#include <QList>
#include <QVector>
#include <QDir>
#include <QTimer>
#include <QDateTime>
#include <QDebug>

#include "filebrowser/file-browser-requests.h"
#include "filebrowser/sharedlink-dialog.h"
#include "filebrowser/seafilelink-dialog.h"
#include "ui/private-share-dialog.h"
#include "rpc/rpc-client.h"
#include "repo-service.h"
#include "api/api-error.h"
#include "seafile-applet.h"
#include "daemon-mgr.h"
#include "account-mgr.h"
#include "settings-mgr.h"
#include "utils/utils.h"
#include "auto-login-service.h"
#include "ext-handler.h"

namespace {

const char *kSeafExtPipeName = "\\\\.\\pipe\\seafile_ext_pipe";
const int kPipeBufSize = 1024;

const quint64 kReposInfoCacheMSecs = 2000;

bool
extPipeReadN (HANDLE pipe, void *buf, size_t len)
{
    DWORD bytes_read;
    bool success = ReadFile(
        pipe,                  // handle to pipe
        buf,                   // buffer to receive data
        (DWORD)len,            // size of buffer
        &bytes_read,           // number of bytes read
        NULL);                 // not overlapped I/O

    if (!success || bytes_read != (DWORD)len) {
        DWORD error = GetLastError();
        if (error == ERROR_BROKEN_PIPE) {
            qDebug("[ext] connection closed by extension\n");
        } else {
            qWarning("[ext] Failed to read command from extension(), "
                     "error code %lu\n", error);
        }
        return false;
    }

    return true;
}

bool
extPipeWriteN(HANDLE pipe, void *buf, size_t len)
{
    DWORD bytes_written;
    bool success = WriteFile(
        pipe,                  // handle to pipe
        buf,                   // buffer to receive data
        (DWORD)len,            // size of buffer
        &bytes_written,        // number of bytes written
        NULL);                 // not overlapped I/O

    if (!success || bytes_written != (DWORD)len) {
        DWORD error = GetLastError();
        if (error == ERROR_BROKEN_PIPE) {
            qDebug("[ext] connection closed by extension\n");
        } else {
            qWarning("[ext] Failed to read command from extension(), "
                     "error code %lu\n", error);
        }
        return false;
    }

    FlushFileBuffers(pipe);
    return true;
}

/**
 * Replace "\" with "/", and remove the trailing slash
 */
QString normalizedPath(const QString& path)
{
    QString p = QDir::fromNativeSeparators(path);
    if (p.endsWith("/")) {
        p = p.left(p.size() - 1);
    }
    return p;
}

std::string formatErrorMessage()
{
    DWORD error_code = ::GetLastError();
    if (error_code == 0) {
        return "no error";
    }
    char buf[256] = {0};
    ::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                    NULL,
                    error_code,
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    buf,
                    sizeof(buf) - 1,
                    NULL);
    return buf;
}

QString repoStatus(const LocalRepo& repo)
{
    QString status = "normal";
    if (!repo.auto_sync) {
        status = "paused";
    } else if (repo.sync_state == LocalRepo::SYNC_STATE_ING) {
        status = "syncing";
    } else if (repo.sync_state == LocalRepo::SYNC_STATE_ERROR) {
        status = "error";
    }

    // qDebug("repo %s (%s, %s): %s", repo.name.toUtf8().data(),
    //        repo.sync_state_str.toUtf8().data(),
    //        repo.sync_error_str.toUtf8().data(),
    //        status.toUtf8().data());

    return status;
}

} // namespace


SINGLETON_IMPL(SeafileExtensionHandler)

SeafileExtensionHandler::SeafileExtensionHandler()
: started_(false)
{
    listener_thread_ = new ExtConnectionListenerThread;

    connect(listener_thread_, SIGNAL(generateShareLink(const QString&, const QString&, bool, bool)),
            this, SLOT(generateShareLink(const QString&, const QString&, bool, bool)));

    connect(listener_thread_, SIGNAL(lockFile(const QString&, const QString&, bool)),
            this, SLOT(lockFile(const QString&, const QString&, bool)));

    connect(listener_thread_, SIGNAL(privateShare(const QString&, const QString&, bool)),
            this, SLOT(privateShare(const QString&, const QString&, bool)));

    connect(listener_thread_, SIGNAL(openUrlWithAutoLogin(const QUrl&)),
            this, SLOT(openUrlWithAutoLogin(const QUrl&)));
}

void SeafileExtensionHandler::start()
{
    listener_thread_->start();
    ReposInfoCache::instance()->start();
    started_ = true;
}

void SeafileExtensionHandler::stop()
{
    if (started_) {
        // Before seafile client exits, tell the shell to clean all the file
        // status icons
        SHChangeNotify (SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
    }
}

void SeafileExtensionHandler::generateShareLink(const QString& repo_id,
                                                const QString& path_in_repo,
                                                bool is_file,
                                                bool internal)
{
    // qDebug("path_in_repo: %s", path_in_repo.toUtf8().data());
    const Account account = seafApplet->accountManager()->getAccountByRepo(repo_id);
    if (!account.isValid()) {
        return;
    }

    if (internal) {
        QString path = path_in_repo;
        if (!is_file && !path.endsWith("/")) {
            path += "/";
        }
        SeafileLinkDialog *dialog = new SeafileLinkDialog(repo_id, account, path, NULL);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->show();
        dialog->raise();
        dialog->activateWindow();
    } else {
        GetSharedLinkRequest *req = new GetSharedLinkRequest(
            account, repo_id, path_in_repo, is_file);

        connect(req, SIGNAL(success(const QString&, const QString&)),
                this, SLOT(onShareLinkGenerated(const QString&)));

        req->send();
    }
}

void SeafileExtensionHandler::lockFile(const QString& repo_id,
                                       const QString& path_in_repo,
                                       bool lock)
{
    // qDebug("path_in_repo: %s", path_in_repo.toUtf8().data());
    const Account account = seafApplet->accountManager()->getAccountByRepo(repo_id);
    if (!account.isValid()) {
        return;
    }

    LockFileRequest *req = new LockFileRequest(
        account, repo_id, path_in_repo, lock);

    connect(req, SIGNAL(success(const QString&)),
            this, SLOT(onLockFileSuccess()));
    connect(req, SIGNAL(failed(const ApiError&)),
            this, SLOT(onLockFileFailed(const ApiError&)));

    req->send();
}

void SeafileExtensionHandler::privateShare(const QString& repo_id,
                                           const QString& path_in_repo,
                                           bool to_group)
{
    const Account account = seafApplet->accountManager()->getAccountByRepo(repo_id);
    if (!account.isValid()) {
        qWarning("no account found for repo %12s", repo_id.toUtf8().data());
        return;
    }

    LocalRepo repo;
    seafApplet->rpcClient()->getLocalRepo(repo_id, &repo);
    PrivateShareDialog *dialog = new PrivateShareDialog(account, repo_id, repo.name,
                                                        path_in_repo, to_group,
                                                        NULL);

    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
    dialog->raise();
    dialog->activateWindow();
}

void SeafileExtensionHandler::openUrlWithAutoLogin(const QUrl& url)
{
    AutoLoginService::instance()->startAutoLogin(url.toString());
}

void SeafileExtensionHandler::onShareLinkGenerated(const QString& link)
{
    SharedLinkDialog *dialog = new SharedLinkDialog(link, NULL);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
    dialog->raise();
    dialog->activateWindow();
}

void SeafileExtensionHandler::onLockFileSuccess()
{
    LockFileRequest *req = qobject_cast<LockFileRequest *>(sender());
    LocalRepo repo;
    seafApplet->rpcClient()->getLocalRepo(req->repoId(), &repo);
    if (repo.isValid()) {
        seafApplet->rpcClient()->markFileLockState(req->repoId(), req->path(), req->lock());
        QString path = QDir::toNativeSeparators(QDir(repo.worktree).absoluteFilePath(req->path().mid(1)));
        SHChangeNotify(SHCNE_ATTRIBUTES, SHCNF_PATH, path.toUtf8().data(), NULL);
    }
}

void SeafileExtensionHandler::onLockFileFailed(const ApiError& error)
{
    LockFileRequest *req = qobject_cast<LockFileRequest *>(sender());
    QString str = req->lock() ? tr("Failed to lock file") : tr("Failed to unlock file");
    seafApplet->warningBox(QString("%1: %2").arg(str, error.toString()));
}


void ExtConnectionListenerThread::run()
{
    while (1) {
        HANDLE pipe = INVALID_HANDLE_VALUE;
        bool connected = false;

        pipe = CreateNamedPipe(
            kSeafExtPipeName,         // pipe name
            PIPE_ACCESS_DUPLEX,       // read/write access
            PIPE_TYPE_MESSAGE |       // message type pipe
            PIPE_READMODE_MESSAGE |   // message-read mode
            PIPE_WAIT,                // blocking mode
            PIPE_UNLIMITED_INSTANCES, // max. instances
            kPipeBufSize,             // output buffer size
            kPipeBufSize,             // input buffer size
            0,                        // client time-out
            NULL);                    // default security attribute

        if (pipe == INVALID_HANDLE_VALUE) {
            qWarning ("Failed to create named pipe, GLE=%lu\n",
                      GetLastError());
            return;
        }

        /* listening on this pipe */
        connected = ConnectNamedPipe(pipe, NULL) ?
            true : (GetLastError() == ERROR_PIPE_CONNECTED);

        if (!connected) {
            qWarning ("Failed on ConnectNamedPipe(), GLE=%lu\n",
                      GetLastError());
            CloseHandle(pipe);
            return;
        }

        qDebug ("[ext pipe] Accepted an extension pipe client\n");
        servePipeInNewThread(pipe);
    }
}

void ExtConnectionListenerThread::servePipeInNewThread(HANDLE pipe)
{
    ExtCommandsHandler *t = new ExtCommandsHandler(pipe);

    connect(t, SIGNAL(generateShareLink(const QString&, const QString&, bool, bool)),
            this, SIGNAL(generateShareLink(const QString&, const QString&, bool, bool)));
    connect(t, SIGNAL(lockFile(const QString&, const QString&, bool)),
            this, SIGNAL(lockFile(const QString&, const QString&, bool)));
    connect(t, SIGNAL(privateShare(const QString&, const QString&, bool)),
            this, SIGNAL(privateShare(const QString&, const QString&, bool)));
    connect(t, SIGNAL(openUrlWithAutoLogin(const QUrl&)),
            this, SIGNAL(openUrlWithAutoLogin(const QUrl&)));
    t->start();
}

ExtCommandsHandler::ExtCommandsHandler(HANDLE pipe)
{
    pipe_ = pipe;
}

void ExtCommandsHandler::run()
{
    while (1) {
        QStringList args;
        if (!readRequest(&args)) {
            qWarning ("failed to read request from shell extension: %s",
                      formatErrorMessage().c_str());
            break;
        }

        QString cmd = args.takeAt(0);
        QString resp;
        if (cmd == "list-repos") {
            resp = handleListRepos(args);
        } else if (cmd == "get-share-link") {
            handleGenShareLink(args, false);
        } else if (cmd == "get-internal-link") {
            handleGenShareLink(args, true);
        } else if (cmd == "get-file-status") {
            resp = handleGetFileStatus(args);
        } else if (cmd == "lock-file") {
            handleLockFile(args, true);
        } else if (cmd == "unlock-file") {
            handleLockFile(args, false);
        } else if (cmd == "private-share-to-group") {
            handlePrivateShare(args, true);
        } else if (cmd == "private-share-to-user") {
            handlePrivateShare(args, false);
        } else if (cmd == "show-history") {
            handleShowHistory(args);
        } else {
            qWarning ("[ext] unknown request command: %s", cmd.toUtf8().data());
        }

        if (!sendResponse(resp)) {
            qWarning ("failed to write response to shell extension: %s",
                      formatErrorMessage().c_str());
            break;
        }
    }

    qDebug ("An extension client is disconnected: GLE=%lu\n",
            GetLastError());
    DisconnectNamedPipe(pipe_);
    CloseHandle(pipe_);
}

bool ExtCommandsHandler::readRequest(QStringList *args)
{
    uint32_t len = 0;
    if (!extPipeReadN(pipe_, &len, sizeof(len)) || len == 0)
        return false;

    QScopedArrayPointer<char> buf(new char[len + 1]);
    buf.data()[len] = 0;
    if (!extPipeReadN(pipe_, buf.data(), len))
        return false;

    QStringList list = QString::fromUtf8(buf.data()).split('\t');
    if (list.empty()) {
        qWarning("[ext] got an empty request");
        return false;
    }
    *args = list;
    return true;
}

bool ExtCommandsHandler::sendResponse(const QString& resp)
{
    QByteArray raw_resp = resp.toUtf8();
    uint32_t len = raw_resp.length();

    if (!extPipeWriteN(pipe_, &len, sizeof(len))) {
        return false;
    }
    if (len > 0) {
        if (!extPipeWriteN(pipe_, raw_resp.data(), len)) {
            return false;
        }
    }
    return true;
}

QList<LocalRepo> ExtCommandsHandler::listLocalRepos(quint64 ts)
{
    return ReposInfoCache::instance()->getReposInfo(ts);
}

void ExtCommandsHandler::handleGenShareLink(const QStringList& args, bool internal)
{
    if (args.size() != 1) {
        return;
    }
    QString path = normalizedPath(args[0]);
    foreach (const LocalRepo& repo, listLocalRepos()) {
        QString wt = normalizedPath(repo.worktree);
        // qDebug("path: %s, repo: %s", path.toUtf8().data(), wt.toUtf8().data());
        if (path.length() > wt.length() && path.startsWith(wt) && path.at(wt.length()) == '/') {
            QString path_in_repo = path.mid(wt.size());
            bool is_file = QFileInfo(path).isFile();
            emit generateShareLink(repo.id, path_in_repo, is_file, internal);
            break;
        }
    }
}

QString ExtCommandsHandler::handleListRepos(const QStringList& args)
{
    if (args.size() != 1) {
        return "";
    }
    bool ok;
    quint64 ts = args[0].toULongLong(&ok);
    if (!ok) {
        return "";
    }

    QStringList infos;
    foreach (const LocalRepo& repo, listLocalRepos(ts)) {
        QStringList fields;
        QString file_lock = repo.account.isAtLeastProVersion(4, 3, 0)
                                ? "file-lock-supported"
                                : "file-lock-unsupported";
        QString private_share = "private-share-supported";;
        if (!repo.account.isPro()) {
            private_share = "private-share-unsupported";
        } else {
            // TODO: Sometimes we can't get the repo info, e.g. when the
            // account the repo belongs to is not the current active account.
            ServerRepo server_repo = RepoService::instance()->getRepo(repo.id);
            if (server_repo.isValid() && server_repo.owner != repo.account.username) {
                private_share = "private-share-unsupported";
            }
        }
        fields << repo.id
               << repo.name
               << normalizedPath(repo.worktree)
               << repoStatus(repo)
               << file_lock
               << private_share;
        infos << fields.join("\t");
    }

    return infos.join("\n");
}

QString ExtCommandsHandler::handleGetFileStatus(const QStringList& args)
{
    if (args.size() != 3) {
        return "";
    }

    QString repo_id = args[0];
    QString path_in_repo = args[1];
    bool isdir = args[2] == "true";
    if (repo_id.length() != 36) {
        return "";
    }

    QString status;
    if (ReposInfoCache::instance()->getRepoFileStatus(repo_id, path_in_repo, isdir, &status)) {
        // qWarning("status for %s is %s", path_in_repo.toUtf8().data(), status.toUtf8().data());
        return status;
    }

    qWarning("failed to get file status for %s", path_in_repo.toUtf8().data());
    return "";
}

void ExtCommandsHandler::handleLockFile(const QStringList& args, bool lock)
{
    if (args.size() != 1) {
        return;
    }
    QString path = normalizedPath(args[0]);
    foreach (const LocalRepo& repo, listLocalRepos()) {
        QString wt = normalizedPath(repo.worktree);
        if (path.length() > wt.length() && path.startsWith(wt) and path.at(wt.length()) == '/') {
            QString path_in_repo = path.mid(wt.size());
            emit lockFile(repo.id, path_in_repo, lock);
            break;
        }
    }
}

void ExtCommandsHandler::handlePrivateShare(const QStringList& args,
                                            bool to_group)
{
    if (args.size() != 1) {
        return;
    }
    QString path = normalizedPath(args[0]);
    if (!QFileInfo(path).isDir()) {
        qWarning("attempted to share %s, which is not a folder",
                 path.toUtf8().data());
        return;
    }
    foreach (const LocalRepo& repo, listLocalRepos()) {
        QString wt = normalizedPath(repo.worktree);
        if (path.length() > wt.length() && path.startsWith(wt) &&
            path.at(wt.length()) == '/') {
            QString path_in_repo = path.mid(wt.size());
            emit privateShare(repo.id, path_in_repo, to_group);
            break;
        }
    }
}

void ExtCommandsHandler::handleShowHistory(const QStringList& args)
{
    if (args.size() != 1) {
        return;
    }
    QString path = normalizedPath(args[0]);
    if (QFileInfo(path).isDir()) {
        qWarning("attempted to view history of %s, which is not a regular file",
                 path.toUtf8().data());
        return;
    }
    foreach (const LocalRepo& repo, listLocalRepos()) {
        QString wt = normalizedPath(repo.worktree);
        if (path.length() > wt.length() && path.startsWith(wt) &&
            path.at(wt.length()) == '/') {
            if (repo.account.isValid()) {
                QString path_in_repo = path.mid(wt.size());
                QUrl url = "/repo/file_revisions/" + repo.id + "/";
                url = ::includeQueryParams(url, {{"p", path_in_repo}});
                emit openUrlWithAutoLogin(url);
            }
            break;
        }
    }
}

SINGLETON_IMPL(ReposInfoCache)

ReposInfoCache::ReposInfoCache(QObject * parent)
    : QObject(parent)
{
    cache_ts_ = 0;
    rpc_client_ = new SeafileRpcClient();
    connect(seafApplet->daemonManager(), SIGNAL(daemonRestarted()), this, SLOT(onDaemonRestarted()));
}

void ReposInfoCache::start()
{
    rpc_client_->tryConnectDaemon();
}

void ReposInfoCache::onDaemonRestarted()
{
    QMutexLocker lock(&rpc_client_mutex_);
    qDebug("reviving message poller when daemon is restarted");
    if (rpc_client_) {
        delete rpc_client_;
    }
    rpc_client_ = new SeafileRpcClient();
    rpc_client_->tryConnectDaemon();
}


QList<LocalRepo> ReposInfoCache::getReposInfo(quint64 ts)
{
    QMutexLocker lock(&rpc_client_mutex_);

    // There are two levels of repos lists cache in the shell extension:
    // 1. The extension would cache the repos list in explorer side so it
    //    doesn't need to queries the applet repeatly in situations like
    //    entering a folder with lots of files
    // 2. The applet would also cache the repos list (in ReposInfoCache), this
    //    is to reduce the overhead when different extension connections askes
    //    for the repos list simultaneously

    quint64 now = QDateTime::currentMSecsSinceEpoch();

    if (cache_ts_ != 0 && cache_ts_ > ts && now - cache_ts_ < kReposInfoCacheMSecs) {
        // qDebug("ReposInfoCache: return cached info");
        return cached_info_;
    }
    // qDebug("ReposInfoCache: fetch from daemon");

    std::vector<LocalRepo> repos;
    rpc_client_->listLocalRepos(&repos);

    for (size_t i = 0; i < repos.size(); i++) {
        LocalRepo& repo = repos[i];
        rpc_client_->getSyncStatus(repo);
        repo.account = seafApplet->accountManager()->getAccountByRepo(repo.id, rpc_client_);
    }

    cached_info_ = QVector<LocalRepo>::fromStdVector(repos).toList();
    cache_ts_ = QDateTime::currentMSecsSinceEpoch();

    return cached_info_;
}

bool ReposInfoCache::getRepoFileStatus(const QString& repo_id,
                                       const QString& path_in_repo,
                                       bool isdir,
                                       QString *status)
{
    QMutexLocker lock(&rpc_client_mutex_);
    return rpc_client_->getRepoFileStatus(repo_id, path_in_repo, isdir, status) == 0;
}
