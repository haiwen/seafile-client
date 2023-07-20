#include <QTimer>
#include <QDateTime>
#include <QJsonDocument>

#include "utils/utils.h"
#include "utils/translate-commit-desc.h"
#include "utils/json-utils.h"
#include "utils/file-utils.h"
#include "seafile-applet.h"
#include "daemon-mgr.h"
#include "settings-mgr.h"
#include "rpc/rpc-client.h"
#include "rpc/sync-error.h"
#include "ui/tray-icon.h"
#include "sync-error-service.h"

#include "message-poller.h"
#if defined(_MSC_VER)
#include "include/seafile-error.h"
#else
#include <seafile/seafile-error.h>
#endif

namespace {

const int kCheckNotificationIntervalMSecs = 1000;

} // namespace

struct SyncNotification {
    QString type;
    QString content;

    static SyncNotification fromJson(const json_t* json);
};


MessagePoller::MessagePoller(QObject *parent): QObject(parent)
{
    rpc_client_ = new SeafileRpcClient();
    check_notification_timer_ = new QTimer(this);
    connect(check_notification_timer_, SIGNAL(timeout()), this, SLOT(checkNotification()));
}

MessagePoller::~MessagePoller()
{
    delete check_notification_timer_;
    delete rpc_client_;
}

void MessagePoller::start()
{
    rpc_client_->tryConnectDaemon();
    check_notification_timer_->start(kCheckNotificationIntervalMSecs);
    connect(seafApplet->daemonManager(), SIGNAL(daemonDead()), this, SLOT(onDaemonDead()));
    connect(seafApplet->daemonManager(), SIGNAL(daemonRestarted()), this, SLOT(onDaemonRestarted()));
}

void MessagePoller::onDaemonDead()
{
    qDebug("pausing message poller when daemon is dead");
    check_notification_timer_->stop();
}

void MessagePoller::onDaemonRestarted()
{
    qDebug("reviving message poller when daemon is restarted");
    if (rpc_client_) {
        delete rpc_client_;
    }
    rpc_client_ = new SeafileRpcClient();
    rpc_client_->tryConnectDaemon();
    check_notification_timer_->start(kCheckNotificationIntervalMSecs);
}

void MessagePoller::checkNotification()
{
    json_t *ret;
    if (!rpc_client_->getSyncNotification(&ret)) {
        return;
    }
    SyncNotification notification = SyncNotification::fromJson(ret);
    json_decref(ret);

    processNotification(notification);
}

SyncNotification SyncNotification::fromJson(const json_t *root)
{
    SyncNotification notification;
    Json json(root);

    // char *s = json_dumps(root, 0);
    // printf ("[%s] %s\n", QDateTime::currentDateTime().toString().toUtf8().data(), s);
    // qWarning ("[%s] %s\n", QDateTime::currentDateTime().toString().toUtf8().data(), s);
    // free (s);

    notification.type = json.getString("type");
    notification.content = json.getString("content");
    return notification;
}

void MessagePoller::processNotification(const SyncNotification& notification)
{
    const QString& type = notification.type;
    const QString& content = notification.content;
    if (type == "transfer") {
        // empty
    } else if (type == "sync.done" ||
               type == "sync.multipart_upload") {
        /* format: a concatenation of (repo_name, repo_id, commmit_id,
         * previous_commit_id, description), separated by tabs */
        QStringList slist = content.split("\t");
        if (slist.count() != 5) {
            qWarning("Bad sync.done message format");
            return;
        }

        QString title;
        if (type == "sync.done")
            title = tr("\"%1\" is synchronized").arg(slist.at(0));
        else
            title = tr("Files uploaded to \"%1\"").arg(slist.at(0));
        QString repo_id = slist.at(1).trimmed();
        QString commit_id = slist.at(2).trimmed();
        QString previous_commit_id = slist.at(3).trimmed();
        QString desc = slist.at(4).trimmed();

        seafApplet->trayIcon()->showMessage(title, translateCommitDesc(desc), repo_id, commit_id, previous_commit_id);

    } else if (type == "sync.error") {
        json_error_t error;
        json_t *object = json_loads(toCStr(content), 0, &error);
        if (!object) {
            qWarning("Failed to parse json: %s", error.text);
            return;
        }

        QString repo_id = QString::fromUtf8(json_string_value(json_object_get(object, "repo_id")));
        QString title = QString::fromUtf8(json_string_value(json_object_get(object, "repo_name")));
        QString path = QString::fromUtf8(json_string_value(json_object_get(object, "path")));
        int err_id = json_integer_value(json_object_get(object, "err_id"));

        switch (err_id) {
        case SYNC_ERROR_ID_FILE_LOCKED_BY_APP:
        case SYNC_ERROR_ID_INDEX_ERROR:
        case SYNC_ERROR_ID_PATH_END_SPACE_PERIOD:
        case SYNC_ERROR_ID_PATH_INVALID_CHARACTER:
        case SYNC_ERROR_ID_UPDATE_TO_READ_ONLY_REPO:
        case SYNC_ERROR_ID_REMOVE_UNCOMMITTED_FOLDER:
#if !defined(Q_OS_WIN32)
        case SYNC_ERROR_ID_INVALID_PATH_ON_WINDOWS:
#endif
            LastSyncError::instance()->flagRepoSyncError(repo_id, err_id);
            break;
        }

        QString msg;
        switch (err_id) {
        case SYNC_ERROR_ID_FILE_LOCKED_BY_APP:
            msg = tr("Failed to sync file %1\nFile is locked by other application. This file will be updated when you close the application.").arg(path);
            break;
        case SYNC_ERROR_ID_FOLDER_LOCKED_BY_APP:
            msg = tr("Failed to sync folder %1\nSome file in this folder is locked by other application. This folder will be updated when you close the application.").arg(path);
            break;
        case SYNC_ERROR_ID_FILE_LOCKED:
            msg = tr("Failed to sync file %1\nFile is locked by another user. Update to this file is not uploaded.").arg(path);
            break;
        case SYNC_ERROR_ID_INVALID_PATH:
            msg = tr("Failed to sync %1\nFile path contains invalid characters. It is not synced to this computer.").arg(path);
            break;
        case SYNC_ERROR_ID_INDEX_ERROR:
            msg = tr("Failed to index file %1\nPlease check file permission and disk space.").arg(path);
            break;
        case SYNC_ERROR_ID_PATH_END_SPACE_PERIOD:
            msg = tr("Failed to sync %1\nFile path is ended with space or period and cannot be created on Windows.").arg(path);
            break;
        case SYNC_ERROR_ID_PATH_INVALID_CHARACTER:
            msg = tr("Failed to sync %1\nFile path contains invalid characters. It is not synced to this computer.").arg(path);
            break;
        case SYNC_ERROR_ID_FOLDER_PERM_DENIED:
            msg = tr("Update to file %1 is denied by folder permission setting.").arg(path);
            break;
        case SYNC_ERROR_ID_PERM_NOT_SYNCABLE:
            msg = tr("Syncing is denied by cloud-only permission settings %1.").arg(path);
            break;
        case SYNC_ERROR_ID_UPDATE_TO_READ_ONLY_REPO:
            msg = tr("Updates in read-only library %1 will not be uploaded.").arg(path);
            break;
        case SYNC_ERROR_ID_CONFLICT:
            msg = tr("Concurrent updates to file. File %1 is saved as conflict file").arg(path);
            break;
        case SYNC_ERROR_ID_REMOVE_UNCOMMITTED_FOLDER:
            msg = tr("Folder %1 is moved to seafile-recycle-bin folder since it contains not-yet uploaded files.").arg(path);
            break;
#if !defined(Q_OS_WIN32)
        case SYNC_ERROR_ID_INVALID_PATH_ON_WINDOWS:
            msg = tr("The file path %1 contains symbols that are not supported by the Windows system.").arg(path);
            break;
#endif
        case SYNC_ERROR_ID_LIBRARY_TOO_LARGE:
            msg = tr("Library is too large to sync.");
            break;
        case SYNC_ERROR_ID_DEL_CONFIRMATION_PENDING:
            msg = tr("Waiting for confirmation to delete files");
            break;
        case SYNC_ERROR_ID_TOO_MANY_FILES:
            msg = tr("Too many files in library");
            break;
        case SYNC_ERROR_ID_CHECKOUT_FILE:
            msg = tr("Failed to checkout file on the client. Please check disk space or folder permissions");
            break;
        default:
            qWarning("Unknown sync error id %d", err_id);
            json_decref(object);
            return;
        }

        seafApplet->trayIcon()->showMessage(title, msg, repo_id,
                                            QString(""), QString(""), QSystemTrayIcon::Information, 10000, true);

        json_decref(object);

    } else if (type == "repo.remove") {
        /* format : <repo_name> */
        QString repo_name = content;
        QString buf = tr("Folder for library %1 is removed or moved. The library is unsynced.").arg(repo_name);
        seafApplet->trayIcon()->showMessage(getBrand(), buf);

    } else if (type == "sync.del_confirmation") {
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(content.toUtf8(), &error);
        if (doc.isNull()) {
            qWarning() << "Failed to parse json:" << error.errorString();
            return;
        } else if (!doc.isObject()) {
            qWarning() << "Malformed json string:" << content;
            return;
        }

        QString text;
        QRegularExpression re("Deleted \"(.+)\" and (.+) more files.");
        auto match = re.match(doc["delete_files"].toString().trimmed());
        if (match.hasMatch()) {
            text = tr("Deleted \"%1\" and %2 more files.")
                      .arg(match.captured(1)).arg(match.captured(2));
        }

        QString info = tr("Do you want to delete files in library \"%1\" ?")
                          .arg(doc["repo_name"].toString().trimmed());

        if (seafApplet->deletingConfirmationBox(text, info)) {
            rpc_client_->addDelConfirmation(doc["confirmation_id"].toString(), false);
        } else {
            rpc_client_->addDelConfirmation(doc["confirmation_id"].toString(), true);
        }
    }
}
