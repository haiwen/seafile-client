
#include <glib.h>

#include <searpc-client.h>
#include <ccnet.h>
#include <searpc.h>
#include <seafile/seafile.h>
#include <seafile/seafile-object.h>

static CcnetClient *sync_client_ = NULL;
static SearpcClient *rpc_client_ = NULL;
static const char *kSeafileRpcService = "seafile-rpcserver";
static const char *get_ccnet_conf_dir ()
{
    static const char *ccnet_dir = NULL;
    if (!ccnet_dir)
    {
        ccnet_dir = g_getenv ("CCNET_CONF_DIR");
        if (!ccnet_dir)
        {
            ccnet_dir = g_build_path ("/", g_get_home_dir (),
                                      ".ccnet", NULL);
        }
    }
    return ccnet_dir;
}

SearpcClient* seafile_rpc_get_instance ()
{
    return rpc_client_;
}

gboolean seafile_rpc_instance_connect ()
{
    const char *ccnet_dir = get_ccnet_conf_dir ();
    if (rpc_client_)
    {
        return TRUE;
    }

    sync_client_ = ccnet_client_new ();
    if (ccnet_client_load_confdir (sync_client_, ccnet_dir) < 0)
    {
        g_object_unref (sync_client_);
        return FALSE;
    }

    if (ccnet_client_connect_daemon (sync_client_, CCNET_CLIENT_SYNC) < 0)
    {
        g_object_unref (sync_client_);
        return FALSE;
    }

    rpc_client_ = ccnet_create_rpc_client (sync_client_, NULL, kSeafileRpcService);
    return TRUE;
}

void seafile_rpc_instance_disconnect ()
{
    if (!rpc_client_)
    {
        return;
    }
    g_free (rpc_client_);
    g_object_unref (sync_client_);
    rpc_client_ = NULL;
    sync_client_ = NULL;
}
