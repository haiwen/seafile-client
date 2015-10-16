#include "shared-application.h"

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#include <event2/event.h>
#include <event2/event_compat.h>
#include <event2/event_struct.h>
#else
#include <event.h>
#endif

extern "C" {
#include <searpc-client.h>
#include <ccnet.h>

#include <searpc.h>
#include <seafile/seafile.h>
#include <seafile/seafile-object.h>

}

#include "utils/utils.h"
namespace {
const char *kAppletCommandsMQ = "applet.commands";
const int kWaitResponseSeconds = 5;
bool isActivate = false;

struct UserData {
    struct event_base *ev_base;
    _CcnetClient *ccnet_client;
    _CcnetMqclientProc *mqclient_proc;
};

void readCallback(int sock, short what, void* data)
{
    UserData *user_data = static_cast<UserData*>(data);
    _CcnetClient *async_client = user_data->ccnet_client;

    //
    // receive message asynchronously
    //
    if (ccnet_client_read_input(async_client) <= 0) {
        // fatal error
        event_base_loopbreak(user_data->ev_base);
    }
}

void messageCallback(CcnetMessage *message, void *data)
{
    isActivate = g_strcmp0(message->body, "ack_activate") == 0;
    if (isActivate) {
        // we get an ack, then break the loop and quit
        struct event_base *ev_base = static_cast<struct event_base*>(data);
        event_base_loopbreak(ev_base);
    }
}

void *askActivateSynchronically() {
    _CcnetClient* async_client = ccnet_client_new();
    const QString ccnet_dir = defaultCcnetDir();
    if (ccnet_client_load_confdir(async_client, NULL, toCStr(ccnet_dir)) <  0) {
        g_object_unref(async_client);
        return 0;
    }

    if (ccnet_client_connect_daemon(async_client, CCNET_CLIENT_ASYNC) < 0) {
        g_object_unref(async_client);
        return 0;
    }

    //
    // set evbase
    //
    struct event_base* ev_base = event_base_new();
    struct timeval timeout;
    timeout.tv_sec = kWaitResponseSeconds;
    timeout.tv_usec = 0;
    event_base_loopexit(ev_base, &timeout);

    //
    // set mqclient proc
    //
    _CcnetMqclientProc *mqclient_proc = (CcnetMqclientProc *)
        ccnet_proc_factory_create_master_processor
        (async_client->proc_factory, "mq-client");

    // set message callback
    ccnet_mqclient_proc_set_message_got_cb(mqclient_proc,
                                           messageCallback,
                                           ev_base);

    // set event callback
    UserData user_data;
    user_data.ev_base = ev_base;
    user_data.ccnet_client = async_client;
    user_data.mqclient_proc = mqclient_proc;
    struct event *read_event = event_new(ev_base,
                                         async_client->connfd,
                                         EV_READ | EV_PERSIST,
                                         readCallback,
                                         &user_data);
    event_add(read_event, NULL);

    const char *topics[] = {
        kAppletCommandsMQ,
    };

    if (ccnet_processor_start((CcnetProcessor *)mqclient_proc,
                              G_N_ELEMENTS(topics), (char **)topics) < 0) {
        event_base_free(ev_base);
        g_object_unref(async_client);
        return 0;
    }

    // send message asynchronously
    CcnetMessage *syn_message = ccnet_message_new(async_client->base.id,
                                                  async_client->base.id,
                                                  kAppletCommandsMQ,
                                                  "syn_activate",
                                                  0);
    ccnet_mqclient_proc_put_message(mqclient_proc, syn_message);
    ccnet_message_free(syn_message);

    //
    // start event loop
    //
    event_base_dispatch(ev_base);

    // cleanup
    event_base_free(ev_base);
    g_object_unref(async_client);

    return 0;
}
} // anonymous namespace

bool SharedApplication::activate() {
    askActivateSynchronically();
    return isActivate;
}
