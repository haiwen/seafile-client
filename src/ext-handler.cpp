#include <winsock2.h>
#include <windows.h>
#include <io.h>
#include <shlwapi.h>
#include <fcntl.h>
#include <ctype.h>
#include <userenv.h>

#include <QMutexLocker>
#include <QScopedPointer>
#include <QList>
#include <QVector>
#include <QDir>
#include <string>

#include "filebrowser/file-browser-requests.h"
#include "filebrowser/sharedlink-dialog.h"
#include "rpc/rpc-client.h"
#include "seafile-applet.h"
#include "account-mgr.h"
#include "ext-handler.h"

namespace {

const char *kSeafExtPipeName = "\\\\.\\pipe\\seafile_ext_pipe";
const int kPipeBufSize = 1024;
const char *kRepoRelayAddrProperty = "relay-address";

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

} // namespace


SINGLETON_IMPL(SeafileExtensionHandler)

SeafileExtensionHandler::SeafileExtensionHandler()
{
    listener_thread_ = new ExtConnectionListenerThread;

    connect(listener_thread_, SIGNAL(generateShareLink(const QString&, const QString&, bool)),
            this, SLOT(generateShareLink(const QString&, const QString&, bool)));
}

void SeafileExtensionHandler::start()
{
    listener_thread_->start();
}

Account SeafileExtensionHandler::findAccountByRepo(const QString& repo_id)
{
    SeafileRpcClient *rpc = seafApplet->rpcClient();
    if (!accounts_cache_.contains(repo_id)) {
        QString relay_addr;
        if (rpc->getRepoProperty(repo_id, kRepoRelayAddrProperty, &relay_addr) < 0) {
            return Account();
        }
        const std::vector<Account>& accounts = seafApplet->accountManager()->accounts();
        for (int i = 0; i < accounts.size(); i++) {
            const Account& account = accounts[i];
            if (account.serverUrl.host() == relay_addr) {
                accounts_cache_[repo_id] = account;
                break;
            }
        }
    }
    return accounts_cache_.value(repo_id, Account());
}

void SeafileExtensionHandler::generateShareLink(const QString& repo_id,
                                                const QString& path_in_repo,
                                                bool is_file)
{
    qDebug("path_in_repo: %s", path_in_repo.toUtf8().data());
    const Account account = findAccountByRepo(repo_id);
    if (!account.isValid()) {
        return;
    }

    GetSharedLinkRequest *req = new GetSharedLinkRequest(
        account, repo_id, path_in_repo, is_file);

    connect(req, SIGNAL(success(const QString&)),
            this, SLOT(onShareLinkGenerated(const QString&)));

    req->send();
}

void SeafileExtensionHandler::onShareLinkGenerated(const QString& link)
{
    SharedLinkDialog *dialog = new SharedLinkDialog(link, NULL);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
    dialog->raise();
    dialog->activateWindow();
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

    connect(t, SIGNAL(generateShareLink(const QString&, const QString&, bool)),
            this, SIGNAL(generateShareLink(const QString&, const QString&, bool)));
    t->start();
}

QMutex ExtCommandsHandler::rpc_mutex_;

ExtCommandsHandler::ExtCommandsHandler(HANDLE pipe)
{
    pipe_ = pipe;
}

void ExtCommandsHandler::run()
{
    rpc_ = new SeafileRpcClient;
    rpc_->connectDaemon();
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
            handleGenShareLink(args);
        } else {
            qWarning ("[ext] unknown request command: %s", cmd.toUtf8().data());
        }

        if (!sendResponse(resp)) {
            qWarning ("failed to write response to shell extension: %s",
                      formatErrorMessage().c_str());
            break;
        }
    }

    qWarning ("An extension client is disconnected: GLE=%lu\n",
              GetLastError());
    DisconnectNamedPipe(pipe_);
    CloseHandle(pipe_);
}

bool ExtCommandsHandler::readRequest(QStringList *args)
{
    uint32_t len;
    if (!extPipeReadN(pipe_, &len, sizeof(len)) || len == 0)
        return false;

    QScopedPointer<char> buf(new char[len + 1]);
    buf.data()[len] = 0;
    if (!extPipeReadN(pipe_, buf.data(), len))
        return false;

    QStringList list = QString::fromUtf8(buf.data()).split('\t', QString::SkipEmptyParts);
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

QList<LocalRepo> ExtCommandsHandler::listLocalRepos()
{
    QMutexLocker lock(&rpc_mutex_);

    std::vector<LocalRepo> repos;
    rpc_->listLocalRepos(&repos);
    return QVector<LocalRepo>::fromStdVector(repos).toList();
}

void ExtCommandsHandler::handleGenShareLink(const QStringList& args)
{
    if (args.size() != 1) {
        return;
    }
    QString path = normalizedPath(args[0]);
    foreach (const LocalRepo& repo, listLocalRepos()) {
        QString wt = normalizedPath(repo.worktree);
        qDebug("path: %s, repo: %s", path.toUtf8().data(), wt.toUtf8().data());
        if (path.length() > wt.length() && path.startsWith(wt) and path.at(wt.length()) == '/') {
            QString path_in_repo = path.mid(wt.size());
            bool is_file = QFileInfo(path).isFile();
            emit generateShareLink(repo.id, path_in_repo, is_file);
            break;
        }
    }
}

QString ExtCommandsHandler::handleListRepos(const QStringList& args)
{
    if (args.size() != 0) {
        return "";
    }
    QStringList infos;
    foreach (const LocalRepo& repo, listLocalRepos()) {
        infos << QString("%1\t%2").arg(repo.id).arg(normalizedPath(repo.worktree));
    }

    return infos.join("\n");
}
