#if defined(Q_OS_WIN)
#include <shellapi.h>
#endif

#include <QDesktopServices>
#include <QUrl>
#include <QVariant>

extern "C" {
#include <searpc-client.h>
#include <ccnet.h>

#include <searpc.h>
#include <seafile/seafile.h>
#include <seafile/seafile-object.h>

}

#include "utils/utils.h"
#include "seafile-applet.h"
#include "ui/main-window.h"
#include "rpc/rpc-client.h"
#include "rpc/local-repo.h"

#include "open-local-helper.h"

namespace {

const char *kAppletCommandsMQ = "applet.commands";
const char *kOpenLocalFilePrefix = "open-local-file\t";
const char *kSeafileProtocolPrefix = "seafile://";

struct LocalFileInfo {
    QString repo_id;
    QString repo_name;
    QString path;

    LocalFileInfo() {}
    LocalFileInfo(QString repo_id, QString repo_name, QString path);

    bool isValid() const { return !repo_id.isEmpty() && !repo_name.isEmpty(); }

    static LocalFileInfo fromEncoded(const QString& url);
};

LocalFileInfo::LocalFileInfo(QString repo_id, QString repo_name, QString path)
    : repo_id(repo_id),
      repo_name(repo_name),
      path(path)
{
}

LocalFileInfo LocalFileInfo::fromEncoded(const QString& url)
{
    QByteArray bytes = QByteArray::fromBase64(url.toUtf8());

    json_error_t error;

    json_t *root = json_loads(bytes.data(), 0, &error);
    if (!root) {
        qDebug("invalid open local url %s\n", toCStr(url));
        return LocalFileInfo();
    }

    QMap<QString, QVariant> dict = mapFromJSON(root, &error);
    json_decref(root);

    QString repo_id, repo_name, path;

    repo_id = dict["repo_id"].toString();
    repo_name = dict["repo_name"].toString();
    path = dict["path"].toString();

    if (repo_id.length() != 36 || repo_name.isEmpty()) {
        qDebug("invalid repo_id %s\n", toCStr(repo_id));
        return LocalFileInfo();
    }

    return LocalFileInfo(repo_id, repo_name, path);
}

} // namespace

OpenLocalHelper* OpenLocalHelper::singleton_ = NULL;

OpenLocalHelper::OpenLocalHelper()
{
    url_ = NULL;
    sync_client_ = NULL;
}

OpenLocalHelper*
OpenLocalHelper::instance()
{
    if (singleton_ == NULL) {
        singleton_ = new OpenLocalHelper;
    }

    return singleton_;
}

bool OpenLocalHelper::connectToCcnetDaemon()
{
    sync_client_ = ccnet_client_new();
    const QString ccnet_dir = defaultCcnetDir();
    if (ccnet_client_load_confdir(sync_client_, toCStr(ccnet_dir)) <  0) {
        g_object_unref (sync_client_);
        sync_client_ = NULL;
        return false;
    }

    if (ccnet_client_connect_daemon(sync_client_, CCNET_CLIENT_SYNC) < 0) {
        g_object_unref (sync_client_);
        sync_client_ = NULL;
        return false;
    }

    return true;
}

void OpenLocalHelper::sendOpenLocalFileMessage(const char *url)
{
    QString content = kOpenLocalFilePrefix;
    content += QString::fromUtf8(url);

    CcnetMessage *open_local_msg;
    open_local_msg = ccnet_message_new (sync_client_->base.id,
                                      sync_client_->base.id,
                                      kAppletCommandsMQ, content.toUtf8().data(), 0);

    ccnet_client_send_message(sync_client_, open_local_msg);

    ccnet_message_free(open_local_msg);
    g_object_unref (sync_client_);
}


void OpenLocalHelper::openLocalFile(const char *url_in)
{
    QString url = QString::fromUtf8(url_in);
    if (url.endsWith("/")) {
        url = url.left(url.length() - 1);
    }

    if (!url.startsWith(kSeafileProtocolPrefix)) {
        return;
    }
    url = url.right(url.length() - strlen(kSeafileProtocolPrefix));


    LocalFileInfo info = LocalFileInfo::fromEncoded(url);
    if (!info.isValid()) {
        return;
    }

    qDebug("[OpenLocalHelper] open local file: repo %s, path %s\n",
           info.repo_id.toUtf8().data(), info.path.toUtf8().data());

    LocalRepo repo;
    if (seafApplet->rpcClient()->getLocalRepo(info.repo_id, &repo) < 0) {
        QString msg = QObject::tr("The library \"%1\" has not been synced yet").arg(info.repo_name);
        messageBox(msg);
    } else {
        QString full_path = QDir(repo.worktree).filePath(info.path);
        if (!QFile(full_path).exists()) {
            qDebug("[OpenLocalHelper] file %s in library %s does not exist, open library folder instead\n",
                   info.path.toUtf8().data(), repo.id.toUtf8().data());
            full_path = repo.worktree;
        }
        if (!QDesktopServices::openUrl(QUrl::fromLocalFile(full_path))) {
            QString file_name = QFileInfo(full_path).fileName();
            QString msg = QObject::tr("%1 couldn't find an application to open file %2").arg(getBrand()).arg(file_name);
            messageBox(msg);
        }
    }
}

void OpenLocalHelper::messageBox(const QString& msg)
{
    MainWindow *win = seafApplet->mainWindow();
    win->show();
    win->raise();
    win->activateWindow();
    seafApplet->messageBox(msg);
}

void OpenLocalHelper::handleOpenLocalFromCommandLine(const char *url)
{
    if (connectToCcnetDaemon()) {
        // An instance of seafile applet is running
        sendOpenLocalFileMessage(url);
        exit(0);
    } else {
        // No instance of seafile client running, we just record the url and
        // let the applet start. The local file will be opened when the applet
        // is ready.
        url_ = strdup(url);
    }
}

void OpenLocalHelper::checkPendingOpenLocalRequest()
{
    if (url_ != NULL) {
        openLocalFile(url_);
        url_ = NULL;
    }
}
