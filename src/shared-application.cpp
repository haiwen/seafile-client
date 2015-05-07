#include "shared-application.h"
#include <QtGlobal>
#ifdef Q_OS_WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif
#ifndef __MSVCRT__
#define __MSVCRT__
#endif
#include <process.h>
#else
#include <pthread.h>
#endif

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
const int kWaitResponseSeconds = 6;
bool isActivate = false;

struct UserData {
    struct event_base *ev_base;
    _CcnetClient *ccnet_client;
};

void readCallback(int sock, short what, void* data)
{
    UserData *user_data = static_cast<UserData*>(data);
    if (ccnet_client_read_input(user_data->ccnet_client) <= 0) {
        // fatal error
        event_base_loopbreak(user_data->ev_base);
    }
}

void messageCallback(CcnetMessage *message, void *data)
{
    isActivate = g_strcmp0(message->body, "ack_activate") == 0;
    if (isActivate) {
        // time to leave
        struct event_base *ev_base = static_cast<struct event_base*>(data);
        event_base_loopbreak(ev_base);
    }
}

#ifndef Q_OS_WIN32
void *askActivateSynchronically(void * /*arg*/) {
#else
unsigned __stdcall askActivateSynchronically(void * /*arg*/) {
#endif
    _CcnetClient* async_client = ccnet_client_new();
    _CcnetClient *sync_client = ccnet_client_new();
    const QString ccnet_dir = defaultCcnetDir();
    if (ccnet_client_load_confdir(async_client, toCStr(ccnet_dir)) <  0) {
        g_object_unref(sync_client);
        g_object_unref(async_client);
        return 0;
    }

    if (ccnet_client_connect_daemon(async_client, CCNET_CLIENT_ASYNC) < 0) {
        g_object_unref(sync_client);
        g_object_unref(async_client);
        return 0;
    }
    if (ccnet_client_load_confdir(sync_client, toCStr(ccnet_dir)) <  0) {
        g_object_unref(sync_client);
        g_object_unref(async_client);
        return 0;
    }

    if (ccnet_client_connect_daemon(sync_client, CCNET_CLIENT_SYNC) < 0) {
        g_object_unref(sync_client);
        g_object_unref(async_client);
        return 0;
    }
    //
    // send message synchronously
    //
    CcnetMessage *syn_message;
    syn_message = ccnet_message_new(sync_client->base.id,
                                    sync_client->base.id,
                                    kAppletCommandsMQ, "syn_activate", 0);

    // blocking io, but cancellable from pthread_cancel
    if (ccnet_client_send_message(sync_client, syn_message) < 0) {
        ccnet_message_free(syn_message);
        g_object_unref(sync_client);
        g_object_unref(async_client);
        return 0;
    }

    ccnet_message_free(syn_message);

    //
    // receive message asynchronously
    //
    struct event_base* ev_base = event_base_new();
    struct timeval timeout;
    timeout.tv_sec = kWaitResponseSeconds - 1;
    timeout.tv_usec = 0;
    // set timeout
    event_base_loopexit(ev_base, &timeout);

    UserData user_data;
    user_data.ev_base = ev_base;
    user_data.ccnet_client = async_client;
    struct event *read_event = event_new(ev_base, async_client->connfd,
                                         EV_READ | EV_PERSIST, readCallback, &user_data);
    event_add(read_event, NULL);

    // set message read callback
    _CcnetMqclientProc *mqclient_proc = (CcnetMqclientProc *)
        ccnet_proc_factory_create_master_processor
        (async_client->proc_factory, "mq-client");

    ccnet_mqclient_proc_set_message_got_cb(mqclient_proc,
                                           messageCallback,
                                           ev_base);

    const char *topics[] = {
        kAppletCommandsMQ,
    };

    if (ccnet_processor_start((CcnetProcessor *)mqclient_proc,
                              G_N_ELEMENTS(topics), (char **)topics) < 0) {
        event_base_free(ev_base);
        g_object_unref(sync_client);
        g_object_unref(async_client);
        return 0;
    }

    event_base_dispatch(ev_base);

    event_base_free(ev_base);
    g_object_unref(sync_client);
    g_object_unref(async_client);
    return 0;
}
} // anonymous namespace

bool SharedApplication::activate() {
#ifndef Q_OS_WIN32 // saddly, this don't work for windows
    pthread_t thread;
    // do it in a background thread
    if (pthread_create(&thread, NULL, askActivateSynchronically, NULL) != 0) {
        return false;
    }
    // keep wait for timeout or thread quiting
    int waiting_count = kWaitResponseSeconds * 10;
    // pthread_kill: currently only a value of 0 is supported on mingw
    while (pthread_kill(thread, 0) == 0 && --waiting_count > 0) {
        msleep(100);
    }
    if (waiting_count == 0) {
        // saddly, pthread_cancel don't work properly on mingw
        pthread_cancel(thread);
    }
#else
    // _beginthreadex as the document says
    // https://msdn.microsoft.com/en-us/library/windows/desktop/ms682453%28v=vs.85%29.aspx
    HANDLE thread = (HANDLE)_beginthreadex(NULL, 0, askActivateSynchronically, NULL, 0, NULL);
    if (!thread)
        return false;
    // keep wait for timeout or thread quiting
    if (WaitForSingleObject(thread, kWaitResponseSeconds) == WAIT_TIMEOUT) {
        TerminateThread(thread, 128);
    }
    CloseHandle(thread);
#endif
    return isActivate;
}
