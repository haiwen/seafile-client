#ifndef H_NAUTILUS_SEAFILE_RPC_CLIENT_H
#define H_NAUTILUS_SEAFILE_RPC_CLIENT_H
#include <glib.h>

G_BEGIN_DECLS
#include <searpc-client.h>

SearpcClient* seafile_rpc_get_instance ();

gboolean seafile_rpc_instance_connect ();
void seafile_rpc_instance_disconnect ();

G_END_DECLS
#endif /* H_NAUTILUS_SEAFILE_RPC_CLIENT_H */
