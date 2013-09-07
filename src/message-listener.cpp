extern "C" {
#include <ccnet.h>
}

#include <QSocketNotifier>
#include <QtDebug>

#include "seafile-applet.h"
#include "settings-mgr.h"
#include "configurator.h"
#include "message-listener.h"
#include "ui/tray-icon.h"
#include "utils/utils.h"


namespace {

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

static bool
collect_transfer_info (QString *msg, const char *info, char *repo_name)
{
    char *p;
    if (! (p = strchr (info, '\t')))
        return false;
    *p = '\0';

    int rate = atoi(p + 1) / 1024;
    QString uploadStr = (strcmp(info, "upload") == 0) ? QObject::tr("Uploading") : QObject::tr("Downloading");
    QString buf = QString("%1 %2, %3 %4 KB/s\n").arg(uploadStr).arg(repo_name).arg(QObject::tr("Speed")).arg(rate);
    msg->append(buf);
    return true;
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

void MessageListener::connectDaemon()
{
    async_client_ = ccnet_client_new();

    const QString config_dir = seafApplet->configurator()->ccnetDir();
    const QByteArray path = config_dir.toUtf8();
    if (ccnet_client_load_confdir(async_client_, path.data()) <  0) {
        seafApplet->errorAndExit(tr("failed to load ccnet config dir ").append(config_dir));
    }

    if (ccnet_client_connect_daemon(async_client_, CCNET_CLIENT_ASYNC) < 0) {
        return;
    }

    socket_notifier_ = new QSocketNotifier(async_client_->connfd, QSocketNotifier::Read);
    connect(socket_notifier_, SIGNAL(activated(int)), this, SLOT(readConnfd()));

    qDebug("[MessageListener] connected to daemon");

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
        "seafile.notification",
    };

    if (ccnet_processor_start ((CcnetProcessor *)mqclient_proc_,
                               G_N_ELEMENTS(topics), (char **)topics) < 0) {

        seafApplet->errorAndExit("Failed to start mq client");
    }
}

void MessageListener::handleMessage(CcnetMessage *message)
{
    qDebug("got a message: %s %s", message->app, message->body);

    char *type = NULL;
    char *content = NULL;

    if (IS_APP_MSG(message, "seafile.heartbeat")) {
    } else if (IS_APP_MSG(message, "seafile.notification")) {
        if (parse_seafile_notification (message->body, &type, &content) < 0)
            return;

        if (strcmp(type, "transfer") == 0) {
            if (!seafApplet->settingsManager()->autoSync())
                return;

            seafApplet->trayIcon()->rotate(true);

            if (!content) {
                qDebug("Handle empty notification");
                return;
            }
            QString buf;
            bool ret = parse_key_value_pairs(content, (KeyValueFunc)collect_transfer_info, &buf);
            qDebug() << "ret="<< ret << buf;
            if (ret)
                seafApplet->trayIcon()->setToolTip(buf);

            return;

        } else if (strcmp(type, "repo.deleted_on_relay") == 0) {
            QString buf = QString("\"%1\" %2").arg(content).arg(tr("is unsynced. \nReason: Deleted on server"));
            seafApplet->trayIcon()->notify("Seafile", buf);
        } else if (strcmp(type, "sync.done") == 0) {
            /* format: repo_name \t repo_id \t description */
            QStringList slist = QString::fromUtf8(content).split("\t");
            if (slist.count() != 3) {
                qDebug("Bad sync.done message format");
                return;
            }

            QString title = QString("\"%1\" %2").arg(slist.at(0)).arg(tr("is synchronized"));
            QString buf = slist.at(2);

            seafApplet->trayIcon()->notify(title, buf);

        } else if (strcmp(type, "sync.access_denied") == 0) {
            /* format: <repo_name\trepo_id> */
            QStringList slist = QString(content).split("\t");
            if (slist.count() != 2) {
                qDebug("Bad sync.access_denied message format");
                return;
            }
            QString buf = QString("\"%1\" %2").arg(slist.at(0)).arg(tr("failed to sync. \nAccess denied to service"));
            seafApplet->trayIcon()->notify("Seafile", buf);

        } else if (strcmp(type, "sync.quota_full") == 0) {
            /* format: <repo_name\trepo_id> */
            QStringList slist = QString(content).split("\t");
            if (slist.count() != 2) {
                qDebug("Bad sync.quota_full message format");
                return;
            }

            QString buf = QString("\"%1\" %2").arg(slist.at(0)).arg(tr("failed to sync.\nThe library owner's storage space is used up."));
            seafApplet->trayIcon()->notify("Seafile", buf);
        }
    }
#ifdef __APPLE__
    else if (strcmp(type, "repo.setwktree") == 0) {
        //seafile_set_repofolder_icns (content);
    } else if  (strcmp(type, "repo.unsetwktree") == 0) {
        //seafile_unset_repofolder_icns (content);
    }
#endif
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
