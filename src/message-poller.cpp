#include <QTimer>
#include <QDateTime>

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

#include "message-poller.h"

namespace {

const int kCheckNotificationIntervalMSecs = 1000;

} // namespace

#define SYNC_ERROR_ID_FILE_LOCKED_BY_APP 0
#define SYNC_ERROR_ID_FOLDER_LOCKED_BY_APP 1
#define SYNC_ERROR_ID_FILE_LOCKED 2
#define SYNC_ERROR_ID_INVALID_PATH 3
#define SYNC_ERROR_ID_INDEX_ERROR 4
#define SYNC_ERROR_ID_END_SPACE_PERIOD 5
#define SYNC_ERROR_ID_INVALID_CHARACTER 6
#define SYNC_ERROR_ID_FOLDER_PERM_DENIED 7

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
    } else if (type == "repo.deleted_on_relay") {
        QString buf = tr("\"%1\" is unsynced. \nReason: Deleted on server").arg(content);
        seafApplet->trayIcon()->showMessage(getBrand(), buf);
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

    } else if (type == "sync.conflict") {
        json_error_t error;
        json_t *object = json_loads(toCStr(content), 0, &error);
        if (!object) {
            qWarning("Failed to parse json: %s", error.text);
            return;
        }

        QString repo_id = QString::fromUtf8(json_string_value(json_object_get(object, "repo_id")));
        QString title = QString::fromUtf8(json_string_value(json_object_get(object, "repo_name")));
        QString path = QString::fromUtf8(json_string_value(json_object_get(object, "path")));
        QString msg = tr("File %1 conflict").arg(path);

        seafApplet->trayIcon()->showMessage(title, msg, repo_id);

        json_decref(object);
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
            // case SYNC_ERROR_ID_INVALID_PATH:
            //     msg = tr("Failed to sync %1\nFile path contains invalid characters. It is not synced to this computer.").arg(path);
            //     break;
        case SYNC_ERROR_ID_INDEX_ERROR:
            msg = tr("Failed to index file %1\nPlease check file permission and disk space.").arg(path);
            break;
        case SYNC_ERROR_ID_END_SPACE_PERIOD:
            msg = tr("Failed to sync %1\nFile path is ended with space or period and cannot be created on Windows.").arg(path);
            break;
        case SYNC_ERROR_ID_INVALID_CHARACTER:
            msg = tr("Failed to sync %1\nFile path contains invalid characters. It is not synced to this computer.").arg(path);
            break;
        case SYNC_ERROR_ID_FOLDER_PERM_DENIED:
            msg = tr("Update to file %1 is denied by folder permission setting.").arg(path);
            break;
        default:
            qWarning("Unknown sync error id %d", err_id);
            json_decref(object);
            return;
        }

        seafApplet->trayIcon()->showMessage(title, msg, repo_id);

        json_decref(object);
    } else if (type == "sync.access_denied") {
        /* format: <repo_name\trepo_id> */
        QStringList slist = content.split("\t");
        if (slist.count() != 2) {
            qWarning("Bad sync.access_denied message format");
            return;
        }
        QString buf = tr("\"%1\" failed to sync. \nAccess denied to service").arg(slist.at(0));
        seafApplet->trayIcon()->showMessage(getBrand(), buf);

    } else if (type == "sync.quota_full") {
        /* format: <repo_name\trepo_id> */
        QStringList slist = content.split("\t");
        if (slist.count() != 2) {
            qWarning("Bad sync.quota_full message format");
            return;
        }

        QString buf = tr("\"%1\" failed to sync.\nThe library owner's storage space is used up.").arg(slist.at(0));
        seafApplet->trayIcon()->showMessage(getBrand(), buf);
    }
}
