extern "C" {
#include <ccnet.h>
}

#include <QSocketNotifier>

#include "seafile-applet.h"
#include "configurator.h"
#include "message-listener.h"

namespace {

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
        seafApplet->errorAndExit(tr("failed to load ccnet config dir %1").arg(config_dir));
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
    qDebug("got a message: %s %s\n", message->app, message->body);
    // TODO: handle message
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
