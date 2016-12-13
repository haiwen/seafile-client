extern "C" {
#include <ccnet.h>
}

#include <jansson.h>

#include <QSocketNotifier>
#include <QCoreApplication>
#include <QtDebug>

#include "utils/utils.h"
#include "seafile-applet.h"
#include "ui/main-window.h"
#include "settings-mgr.h"
#include "configurator.h"
#include "ui/tray-icon.h"
#include "utils/utils.h"
#include "utils/translate-commit-desc.h"
#include "open-local-helper.h"

#include "message-listener.h"

namespace {

const char *kSeafileNotificationsMQ = "seafile.notification";
const char *kAppletCommandsMQ = "applet.commands";

#define SYNC_ERROR_ID_FILE_LOCKED_BY_APP 0
#define SYNC_ERROR_ID_FOLDER_LOCKED_BY_APP 1
#define SYNC_ERROR_ID_FILE_LOCKED 2
#define SYNC_ERROR_ID_INVALID_PATH 3
#define SYNC_ERROR_ID_INDEX_ERROR 4
#define SYNC_ERROR_ID_END_SPACE_PERIOD 5
#define SYNC_ERROR_ID_INVALID_CHARACTER 6

#define IS_APP_MSG(msg,topic) (strcmp((msg)->app, (topic)) == 0)
static int parse_seafile_notification (char *msg, char **type, char **body)
{
    if (!msg)
        return -1;

    char *ptr = strchr (msg, '\n');
    if (!ptr)
        return -1;

    *ptr = '\0';

    *type = msg;
    *body = ptr + 1;

    return 0;
}

/**
 * Wrapper callback for mq-client
 */
void messageCallback(CcnetMessage *message, void *data)
{
    MessageListener *listener = static_cast<MessageListener*>(data);
    listener->handleMessage(message);
}

} // namespace


MessageListener::MessageListener()
    : async_client_(0),
      mqclient_proc_(0),
      socket_notifier_(0)
{
}

MessageListener::~MessageListener()
{
    if (socket_notifier_) {
        delete socket_notifier_;
    }
}

void MessageListener::connectDaemon()
{
    async_client_ = ccnet_client_new();

    const QString config_dir = seafApplet->configurator()->ccnetDir();
    const QByteArray path = config_dir.toUtf8();
    if (ccnet_client_load_confdir(async_client_, NULL, path.data()) <  0) {
        seafApplet->errorAndExit(tr("failed to load ccnet config dir ").append(config_dir));
    }

    if (ccnet_client_connect_daemon(async_client_, CCNET_CLIENT_ASYNC) < 0) {
        return;
    }

    socket_notifier_ = new QSocketNotifier(async_client_->connfd, QSocketNotifier::Read);
    connect(socket_notifier_, SIGNAL(activated(int)), this, SLOT(readConnfd()));

    qWarning("[MessageListener] connected to daemon");

    startMqClient();
}

void MessageListener::startMqClient()
{
    mqclient_proc_ = (CcnetMqclientProc *)
        ccnet_proc_factory_create_master_processor
        (async_client_->proc_factory, "mq-client");

    ccnet_mqclient_proc_set_message_got_cb (mqclient_proc_,
                                            (MessageGotCB)messageCallback,
                                            this);

    static const char *topics[] = {
        // "seafile.heartbeat",
        kSeafileNotificationsMQ,
        kAppletCommandsMQ,
    };

    if (ccnet_processor_start ((CcnetProcessor *)mqclient_proc_,
                               G_N_ELEMENTS(topics), (char **)topics) < 0) {

        seafApplet->errorAndExit("Failed to start mq client");
    }
}

void MessageListener::handleMessage(CcnetMessage *message)
{
    // qWarning("got a message: %s %s.", message->app, message->body);

    char *type = NULL;
    char *content = NULL;

    if (IS_APP_MSG(message, kAppletCommandsMQ)) {
        if (g_strcmp0(message->body, "quit") == 0) {
            qWarning("[Message Listener] Got a quit command. Quit now.");
            QCoreApplication::exit(0);
            return;
        }
        if (g_strcmp0(message->body, "syn_activate") == 0) {
            qWarning("[Message Listener] Got an activate command.");
            CcnetMessage *ack_message;
            ack_message = ccnet_message_new(async_client_->base.id,
                                            async_client_->base.id,
                                            kAppletCommandsMQ, "ack_activate", 0);
            ccnet_mqclient_proc_put_message(mqclient_proc_, ack_message);
            ccnet_message_free(ack_message);

            seafApplet->mainWindow()->showWindow();
        }

        const char *kOpenLocalFilePrefix = "open-local-file\t";
        if (strstr(message->body, kOpenLocalFilePrefix) == message->body) {
            OpenLocalHelper::instance()->openLocalFile(QUrl::fromEncoded(message->body + strlen(kOpenLocalFilePrefix)));
        }

    } else if (IS_APP_MSG(message, kSeafileNotificationsMQ)) {
        if (parse_seafile_notification (message->body, &type, &content) < 0)
            return;

        if (strcmp(type, "transfer") == 0) {
            // empty
        } else if (strcmp(type, "repo.deleted_on_relay") == 0) {
            QString buf = tr("\"%1\" is unsynced. \nReason: Deleted on server").arg(QString::fromUtf8(content));
            seafApplet->trayIcon()->showMessage(getBrand(), buf);
        } else if (strcmp(type, "sync.done") == 0) {
            /* format: a concatenation of (repo_name, repo_id, commmit_id,
             * previous_commit_id, description), separated by tabs */
            QStringList slist = QString::fromUtf8(content).split("\t");
            if (slist.count() != 5) {
                qWarning("Bad sync.done message format");
                return;
            }

            QString title = tr("\"%1\" is synchronized").arg(slist.at(0));
            QString repo_id = slist.at(1).trimmed();
            QString commit_id = slist.at(2).trimmed();
            QString previous_commit_id = slist.at(3).trimmed();
            QString desc = slist.at(4).trimmed();

            seafApplet->trayIcon()->showMessage(title, translateCommitDesc(desc), repo_id, commit_id, previous_commit_id);

        } else if (strcmp(type, "sync.conflict") == 0) {
            json_error_t error;
            json_t *object = json_loads(content, 0, &error);
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
        } else if (strcmp(type, "sync.error") == 0) {
            json_error_t error;
            json_t *object = json_loads(content, 0, &error);
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
                msg = tr("Failed to sync file %1\nFile is locked by other user on the server. Update to this file is not uploaded.").arg(path);
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
            default:
                qWarning("Unknown sync error id %d", err_id);
                json_decref(object);
                return;
            }

            seafApplet->trayIcon()->showMessage(title, msg, repo_id);

            json_decref(object);
        } else if (strcmp(type, "sync.access_denied") == 0) {
            /* format: <repo_name\trepo_id> */
            QStringList slist = QString::fromUtf8(content).split("\t");
            if (slist.count() != 2) {
                qWarning("Bad sync.access_denied message format");
                return;
            }
            QString buf = tr("\"%1\" failed to sync. \nAccess denied to service").arg(slist.at(0));
            seafApplet->trayIcon()->showMessage(getBrand(), buf);

        } else if (strcmp(type, "sync.quota_full") == 0) {
            /* format: <repo_name\trepo_id> */
            QStringList slist = QString::fromUtf8(content).split("\t");
            if (slist.count() != 2) {
                qWarning("Bad sync.quota_full message format");
                return;
            }

            QString buf = tr("\"%1\" failed to sync.\nThe library owner's storage space is used up.").arg(slist.at(0));
            seafApplet->trayIcon()->showMessage(getBrand(), buf);
#if defined(Q_OS_MAC)
        } else if (strcmp(type, "repo.setwktree") == 0) {
            //seafile_set_repofolder_icns (content);
        } else if (strcmp(type, "repo.unsetwktree") == 0) {
            //seafile_unset_repofolder_icns (content);
#endif
        }
    }
}

void MessageListener::readConnfd()
{
    socket_notifier_->setEnabled(false);
    if (ccnet_client_read_input(async_client_) <= 0) {
        // network error
        return;
    } else {
        socket_notifier_->setEnabled(true);
    }
}
