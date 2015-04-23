#include "shared-application.h"
#include <QtGlobal>
#ifdef Q_OS_WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif
#endif

extern "C" {
#include <searpc-client.h>
#include <ccnet.h>

#include <searpc.h>
#include <seafile/seafile.h>
#include <seafile/seafile-object.h>

}
#include <pthread.h>

#include "utils/utils.h"
namespace {
const char *kAppletCommandsMQ = "applet.commands";
bool isActivate = false;

#ifndef Q_OS_WIN32
void *askActivateSynchronically(void * /*arg*/) {
#else
DWORD WINAPI askActivateSynchronically(LPVOID /*arg*/) {
#endif
    CcnetClient *sync_client = ccnet_client_new();
    const QString ccnet_dir = defaultCcnetDir();
    if (ccnet_client_load_confdir(sync_client, toCStr(ccnet_dir)) <  0) {
        g_object_unref(sync_client);
        return 0;
    }

    if (ccnet_client_connect_daemon(sync_client, CCNET_CLIENT_SYNC) < 0) {
        g_object_unref(sync_client);
        return 0;
    }

    CcnetMessage *syn_message;
    syn_message = ccnet_message_new(sync_client->base.id,
                                    sync_client->base.id,
                                    kAppletCommandsMQ, "syn_activate", 0);

    // blocking io, but cancellable from pthread_cancel
    if (ccnet_client_send_message(sync_client, syn_message) < 0) {
        ccnet_message_free(syn_message);
        g_object_unref(sync_client);
        return 0;
    }

    ccnet_message_free(syn_message);

    CcnetMessage *ack_message;
    // blocking io, but cancellable from pthread_cancel
    if (ccnet_client_prepare_recv_message(sync_client, kAppletCommandsMQ) < 0 ||
        ((ack_message = ccnet_client_receive_message(sync_client)) == NULL)) {
        g_object_unref(sync_client);
        return 0;
    }
    isActivate = g_strcmp0(ack_message->body, "ack_activate") == 0;
    ccnet_message_free(ack_message);

    g_object_unref(sync_client);
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
    int waiting_ms = 10;
    // pthread_kill: currently only a value of 0 is supported on mingw
    while (pthread_kill(thread, 0) == 0 && --waiting_ms > 0) {
        msleep(100);
    }
    if (waiting_ms == 0) {
        pthread_cancel(thread);
    }
#else
    HANDLE thread = CreateThread(NULL, 0, askActivateSynchronically, NULL, 0, NULL);
    if (!thread)
        return false;
    // keep wait for timeout or thread quiting
    if (WaitForSingleObject(thread, 1000) == WAIT_TIMEOUT) {
        TerminateThread(thread, 128);
    }
    CloseHandle(thread);
#endif
    return isActivate;
}
