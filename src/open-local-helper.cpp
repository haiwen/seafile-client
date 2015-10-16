#if defined(Q_OS_WIN32)
#include <shellapi.h>
#endif

#include <QDesktopServices>
#include <QUrl>
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QUrlQuery>
#endif
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

#include "account.h"
#include "repo-service.h"
#include "open-local-helper.h"

namespace {

const char *kAppletCommandsMQ = "applet.commands";
const char *kOpenLocalFilePrefix = "open-local-file\t";
const char *kSeafileProtocolScheme = "seafile";
const char *kSeafileProtocolHostOpenFile = "openfile";


} // namespace

OpenLocalHelper* OpenLocalHelper::singleton_ = NULL;

OpenLocalHelper::OpenLocalHelper()
{
    url_ = NULL;
    sync_client_ = NULL;

    QDesktopServices::setUrlHandler(kSeafileProtocolScheme, this, SLOT(openLocalFile(const QUrl&)));
}

OpenLocalHelper*
OpenLocalHelper::instance()
{
    if (singleton_ == NULL) {
        static OpenLocalHelper instance;
        singleton_ = &instance;
    }

    return singleton_;
}

bool OpenLocalHelper::connectToCcnetDaemon()
{
    sync_client_ = ccnet_client_new();
    const QString ccnet_dir = defaultCcnetDir();
    if (ccnet_client_load_confdir(sync_client_, NULL, toCStr(ccnet_dir)) <  0) {
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

QUrl OpenLocalHelper::generateLocalFileSeafileUrl(const QString& repo_id, const Account& account, const QString& path)
{
    QUrl url;
    url.setScheme(kSeafileProtocolScheme);
    url.setHost(kSeafileProtocolHostOpenFile);

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    QUrlQuery url_query;
    url_query.addQueryItem("repo_id",  repo_id);
    url_query.addQueryItem("path",  path);
    url.setQuery(url_query);
#else
    url.addQueryItem("repo_id",  repo_id);
    url.addQueryItem("path",  path);
#endif

    return url;
}

QUrl OpenLocalHelper::generateLocalFileWebUrl(const QString& repo_id, const Account& account, const QString& path)
{
    QString fixed_path = path.startsWith("/") ? path : "/" + path;
    if (fixed_path.endsWith("/"))
        return account.getAbsoluteUrl("/#common/lib/" + repo_id + (fixed_path == "/" ?
                                      "/" : fixed_path.left(fixed_path.size() - 1)));
    else
        return account.getAbsoluteUrl("/lib/" + repo_id + "/file" + fixed_path);
}

bool OpenLocalHelper::openLocalFile(const QUrl &url)
{
    if (url.scheme() != kSeafileProtocolScheme) {
        qWarning("[OpenLocalHelper] unknown scheme %s\n", url.scheme().toUtf8().data());
        return false;
    }

    if (url.host() != kSeafileProtocolHostOpenFile) {
        qWarning("[OpenLocalHelper] unknown command %s\n", url.host().toUtf8().data());
        return false;
    }

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    QUrlQuery url_query = QUrlQuery(url.query());
    QString repo_id = url_query.queryItemValue("repo_id", QUrl::FullyDecoded);
    QString email = url_query.queryItemValue("email", QUrl::FullyDecoded);
    QString path = url_query.queryItemValue("path", QUrl::FullyDecoded);
#else
    QString repo_id = url.queryItemValue("repo_id");
    QString email = url.queryItemValue("email");
    QString path = url.queryItemValue("path");
#endif

    if (repo_id.size() < 36) {
        qWarning("[OpenLocalHelper] invalid repo_id %s\n", repo_id.toUtf8().data());
        return false;
    }

    qDebug("[OpenLocalHelper] open local file: repo %s, path %s\n",
           repo_id.toUtf8().data(), path.toUtf8().data());

    RepoService::instance()->openLocalFile(repo_id, path);

    return true;
}

void OpenLocalHelper::messageBox(const QString& msg)
{
    seafApplet->mainWindow()->showWindow();
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
        setUrl(url);
    }
}

void OpenLocalHelper::checkPendingOpenLocalRequest()
{
    if (!url_.isEmpty()) {
        openLocalFile(QUrl::fromEncoded(url_));
        setUrl(NULL);
    }
}
