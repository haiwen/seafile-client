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

#include "filebrowser/file-browser-requests.h"
#include "filebrowser/sharedlink-dialog.h"
#include "rpc/rpc-client.h"
#include "seafile-applet.h"
#include "account-mgr.h"
#include "ext-handler.h"

namespace {

const char *kSeafExtPipeName = "\\\\.\\pipe\\seafile_ext_pipe";
const int kPipeBufSize = 1024;

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
QString uniformPath(const QString& path)
{
    QString p = QDir::fromNativeSeparators(path);
    if (p.endsWith("/")) {
        p = p.left(p.size() - 1);
    }
    return p;
}

} // namespace


SINGLETON_IMPL(SeafileExtensionHandler)

SeafileExtensionHandler::SeafileExtensionHandler()
{
    handler_thread_ = new ExtHandlerThread;

    connect(handler_thread_, SIGNAL(generateShareLink(const QString&, const QString&, bool)),
            this, SLOT(generateShareLink(const QString&, const QString&, bool)));
}

void SeafileExtensionHandler::start()
{
    handler_thread_->start();
}

Account SeafileExtensionHandler::findAccountByRepo(const QString& repo_id)
{
    // TODO: get the account the repo belongs to
    return seafApplet->accountManager()->currentAccount();
    // return Account();
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
    SharedLinkDialog(link, NULL).exec();
}

void ExtHandlerThread::run()
{
    rpc_ = new SeafileRpcClient;
    rpc_->connectDaemon();
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
        /* TODO: use a seperate thread to communicate with this pipe client */
        handleOneConnection(pipe);
    }
}

void ExtHandlerThread::handleOneConnection(HANDLE pipe)
{
    uint32_t len;
    char *buf;
    char *reply;

    qDebug("ExtHandlerThread::handleOneConnection() called");

    while (1) {
        if (!extPipeReadN(pipe, &len, sizeof(len)) || len == 0)
            break;

        QScopedPointer<char> buf(new char[len + 1]);
        if (!extPipeReadN(pipe, buf.data(), len))
            break;

        buf.data()[len] = 0;

        qWarning("[ext] get command: %s\n", buf.data());

        handleGenShareLink(uniformPath(buf.data()));

        // reply = ext_handle_input (buf);
        // if (!reply)
        //     continue;
        // len = strlen(reply) + 1;

        // if (ext_pipe_writen(pipe, &len, sizeof(len)) < 0) {
        //     g_free (reply);
        //     break;
        // }

        // if (ext_pipe_writen(pipe, reply, len) < 0) {
        //     g_free (reply);
        //     break;
        // }

        // g_free (reply);
    }

    // ext_pipe_on_exit(pipe);
}


QList<LocalRepo> ExtHandlerThread::listLocalRepos()
{
    QMutexLocker lock(&mutex_);

    std::vector<LocalRepo> repos;
    rpc_->listLocalRepos(&repos);
    return QVector<LocalRepo>::fromStdVector(repos).toList();
}

void ExtHandlerThread::handleGenShareLink(const QString& path)
{
    foreach (const LocalRepo& repo, listLocalRepos()) {
        QString wt = uniformPath(repo.worktree);
        qDebug("path: %s, repo: %s", path.toUtf8().data(), wt.toUtf8().data());
        if (path.startsWith(wt)) {
            QString path_in_repo = path.mid(wt.size());
            bool is_file = QFileInfo(path).isFile();
            emit generateShareLink(repo.id, path_in_repo, is_file);
        }
    }
}
