#if defined(Q_WS_WIN)
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

    static LocalFileInfo fromEncoded(const char *);
};

LocalFileInfo::LocalFileInfo(QString repo_id, QString repo_name, QString path)
    : repo_id(repo_id),
      repo_name(repo_name),
      path(path)
{
}

LocalFileInfo LocalFileInfo::fromEncoded(const char *url)
{
    QByteArray bytes = QByteArray::fromBase64(url);

    json_error_t error;

    json_t *root = json_loads(bytes.data(), 0, &error);
    if (!root) {
        return LocalFileInfo();
    }

    QMap<QString, QVariant> dict = mapFromJSON(root, &error);
    json_decref(root);

    QString repo_id, repo_name, path;

    repo_id = dict["repo_id"].toString();
    repo_name = dict["repo_name"].toString();
    path = dict["path"].toString();

    if (repo_id.length() != 36 || repo_name.isEmpty()) {
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


void OpenLocalHelper::openLocalFile(const char *url)
{
    if (!QString::fromUtf8(url).startsWith(kSeafileProtocolPrefix)) {
        qDebug("");
        return;
    }

    LocalFileInfo info = LocalFileInfo::fromEncoded(url + strlen(kSeafileProtocolPrefix));
    if (!info.isValid()) {
        return;
    }

    printf("[OpenLocalHelper] open local file: repo %s, path %s\n",
           info.repo_id.toUtf8().data(), info.path.toUtf8().data());
    qDebug("[OpenLocalHelper] open local file: repo %s, path %s\n",
           info.repo_id.toUtf8().data(), info.path.toUtf8().data());

    LocalRepo repo;
    if (seafApplet->rpcClient()->getLocalRepo(info.repo_id, &repo) < 0) {
        QString msg = QObject::tr("The library \"%1\" has not been downloaded yet").arg(info.repo_name);
        MainWindow *win = seafApplet->mainWindow();
        win->show();
        win->raise();
        win->activateWindow();
        seafApplet->messageBox(msg);
    } else {
        QString full_path = QDir(repo.worktree).filePath(info.path);
        if (!QFile(full_path).exists()) {
            printf("[OpenLocalHelper] file %s in library %s does not exist, open library folder instead\n",
                   info.path.toUtf8().data(), repo.id.toUtf8().data());
            qDebug("[OpenLocalHelper] file %s in library %s does not exist, open library folder instead\n",
                   info.path.toUtf8().data(), repo.id.toUtf8().data());
            full_path = repo.worktree;
        }
        QDesktopServices::openUrl(QUrl::fromLocalFile(full_path));
    }
}

void OpenLocalHelper::handleOpenLocalFromCommandLine(const char *url)
{
    if (connectToCcnetDaemon()) {
        // An instance of seafile applet is running
        printf ("applet is running, send a message to open %s\n", url);
        sendOpenLocalFileMessage(url);
        exit(0);
    } else {
        // No instance of seafile client running, we just record the url and
        // let the applet start. The local file will be opened when the applet
        // is ready.
        printf ("applet is not running, record url %s\n", url);
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
