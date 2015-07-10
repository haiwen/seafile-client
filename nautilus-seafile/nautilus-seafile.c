#include "nautilus-seafile.h"
#include <string.h>
#include <libnautilus-extension/nautilus-menu-provider.h>
#include <libnautilus-extension/nautilus-column-provider.h>
#include <libnautilus-extension/nautilus-info-provider.h>
#include <gtk/gtk.h>
#include <searpc-client.h>
#include <seafile/seafile.h>
#include <seafile/seafile-object.h>

#include "seafile-rpc-client.h"

static guint kAutoconnectInterval = 10 * 1000;
static guint kUpdateWatchSetInterval = 4 * 1000;
static guint kUpdateFileStatusInterval = 2 * 1000;
static void start_autoconnect_rpc_client ();
static GObject* find_repo_in_list (const char* path, GList *head);
static gboolean update_file_status_by_file (SearpcClient *client, NautilusFileInfo *file, gboolean update_file_info);
static void clean_up_single_file (gpointer data, GObject *obj);
static void clean_up_files_in_worktree(const char *worktree);
static void clean_up_files_in_old_watch_set (GList *watch_set, GList *new_watch_set);
static void clean_up_all_data ();

static GList *watch_set_ = NULL;
static GHashTable *file_status_ = NULL;

static const char* kSeafileSyncStatus = "SeafileExtension::sync_status";
static const char* kSeafileSyncStatusColumnName = "SeafileExtension::sync_status_column";

/* used on start up only */
static void initialize_work ();

static GList *seafile_extension_get_background_items (NautilusMenuProvider *provider, GtkWidget *window, NautilusFileInfo *file_info)
{
    return NULL;
}

static GList *seafile_extension_get_file_items (NautilusMenuProvider *provider, GtkWidget *window, GList *files)
{
    /* TODO implement */
    return NULL;
}

static GList *seafile_extension_get_columns (NautilusColumnProvider *provider)
{
    NautilusColumn *column;
    GList *ret;
    column = nautilus_column_new (kSeafileSyncStatusColumnName,
                                  kSeafileSyncStatus,
                                  "Sync Status",
                                  "Sync Status Description");
    ret = g_list_append (NULL, column);

    return ret;
}

static NautilusOperationResult seafile_extension_update_file_info (NautilusInfoProvider     *provider,
                                                                   NautilusFileInfo         *file,
                                                                   GClosure                 *update_complete,
                                                                   NautilusOperationHandle **handle)
{
    char *uri = NULL;
    gchar *filename = NULL;
    SearpcClient *client;
    GObject *repo;

    (void)update_complete; /* unused */
    (void)handle; /* unused */

    client = seafile_rpc_get_instance ();
    if (!client)
    {
        return NAUTILUS_OPERATION_COMPLETE;
    }
    g_return_val_if_fail (file, NAUTILUS_OPERATION_FAILED);
    g_return_val_if_fail (!nautilus_file_info_is_gone (file), NAUTILUS_OPERATION_COMPLETE);

    uri = nautilus_file_info_get_uri (file);
    filename = g_filename_from_uri (uri, NULL, NULL);
    if (!filename)
    {
        g_free (uri);
        return NAUTILUS_OPERATION_COMPLETE;
    }

    repo = find_repo_in_list (filename, watch_set_);
    if (!repo)
    {
        g_free (filename);
        g_free (uri);
        return NAUTILUS_OPERATION_COMPLETE;
    }
    /* we can change it to a asynchronize way if we want? */
    update_file_status_by_file (client, file, TRUE);

	g_object_weak_ref (G_OBJECT (file), clean_up_single_file, NULL);

    g_free (filename);
    g_free (uri);

    return NAUTILUS_OPERATION_COMPLETE;
}

static void seafile_extension_cancel_update (NautilusInfoProvider     *provider,
                                             NautilusOperationHandle  *handle)
{
    /* Nothing */
}

static void
seafile_extension_menu_provider_iface_init (NautilusMenuProviderIface *iface)
{
    iface->get_background_items = seafile_extension_get_background_items;
    iface->get_file_items = seafile_extension_get_file_items;
}

static void
seafile_extension_column_provider_iface_init (NautilusColumnProviderIface *iface)
{
    iface->get_columns = seafile_extension_get_columns;
    return;
}

static void
seafile_extension_info_provider_iface_init (NautilusInfoProviderIface *iface)
{
    iface->update_file_info = seafile_extension_update_file_info;
    iface->cancel_update = seafile_extension_cancel_update;
}

static void
seafile_extension_instance_init (SeafileExtension *seafile)
{
}

static void
seafile_extension_class_init (SeafileExtensionClass *class)
{
}

static GType seafile_type = 0;

GType
seafile_extension_get_type (void)
{
    return seafile_type;
}

void
seafile_extension_register_type (GTypeModule *module)
{
    static const GTypeInfo info = {
         sizeof (SeafileExtensionClass),
         (GBaseInitFunc) NULL,
         (GBaseFinalizeFunc) NULL,
         (GClassInitFunc) seafile_extension_class_init,
         NULL,
         NULL,
         sizeof (SeafileExtension),
         0,
         (GInstanceInitFunc) seafile_extension_instance_init,
         NULL
    };

    static const GInterfaceInfo menu_provider_iface_info = {
        (GInterfaceInitFunc) seafile_extension_menu_provider_iface_init,
        NULL,
        NULL,
    };

    static const GInterfaceInfo column_provider_iface_info = {
        (GInterfaceInitFunc) seafile_extension_column_provider_iface_init,
        NULL,
        NULL
    };

    static const GInterfaceInfo info_provider_iface_info = {
        (GInterfaceInitFunc) seafile_extension_info_provider_iface_init,
        NULL,
        NULL,
    };

    seafile_type = g_type_module_register_type (module, G_TYPE_OBJECT, "seafileExtension", &info, 0);

    g_type_module_add_interface (module,
                     seafile_type,
                     NAUTILUS_TYPE_MENU_PROVIDER,
                     &menu_provider_iface_info);

    g_type_module_add_interface (module,
                                 seafile_type,
                                 NAUTILUS_TYPE_COLUMN_PROVIDER,
                                 &column_provider_iface_info);

    g_type_module_add_interface (module,
                     seafile_type,
                     NAUTILUS_TYPE_INFO_PROVIDER,
                     &info_provider_iface_info);

    initialize_work ();
}

static gboolean update_file_status (gpointer data);
static gboolean is_file_status_started = FALSE;
static void stop_update_file_status (gpointer data)
{
    /* we get disconnected */
    is_file_status_started = FALSE;
}

static void start_update_file_status ()
{
    g_return_if_fail (!is_file_status_started);
    is_file_status_started = TRUE;
    g_timeout_add_full (G_PRIORITY_DEFAULT_IDLE, kUpdateFileStatusInterval, update_file_status, NULL, stop_update_file_status);
}

static gboolean is_watch_set_started = FALSE;
static void stop_update_watch_set (gpointer data)
{
    is_watch_set_started = FALSE;
    /* we get disconnected */
    clean_up_all_data ();
    start_autoconnect_rpc_client ();
}

static gboolean update_watch_set (gpointer data);
static void start_update_watch_set ()
{
    g_return_if_fail (!is_watch_set_started);
    is_watch_set_started = TRUE;
    g_timeout_add_full (G_PRIORITY_DEFAULT_IDLE, kUpdateWatchSetInterval, update_watch_set, NULL, stop_update_watch_set);
    start_update_file_status ();
}

static gboolean is_connect_rpc_started = FALSE;
static void stop_connect_rpc_client (gpointer data)
{
    is_connect_rpc_started = FALSE;

    start_update_watch_set ();
}

static gboolean connect_rpc_client (gpointer data);
static void start_autoconnect_rpc_client ()
{
    seafile_rpc_instance_disconnect ();

    g_return_if_fail (!is_connect_rpc_started);
    is_connect_rpc_started = TRUE;
    g_timeout_add_full (G_PRIORITY_DEFAULT_IDLE, kAutoconnectInterval, connect_rpc_client, NULL, stop_connect_rpc_client);
}

static void initialize_work ()
{
    file_status_ = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

    if (connect_rpc_client (NULL))
    {
        start_autoconnect_rpc_client ();
    }
    else
    {
        stop_connect_rpc_client (NULL);
    }
}

static gboolean connect_rpc_client (gpointer data)
{
    /* connected? */
    g_return_val_if_fail (!seafile_rpc_instance_connect (), FALSE);

    /* should we retry connect */
    return TRUE;
}

static inline gboolean update_file_status_by_name (SearpcClient *client, const char* path)
{
    gboolean retval;
    GFile *gfile;
    NautilusFileInfo *file;
    gfile = g_file_new_for_path (path);
    g_return_val_if_fail (gfile, TRUE);
    file = nautilus_file_info_lookup (gfile);
    if (!file)
    {
        g_object_unref (gfile);
        return TRUE;
    }
    retval = update_file_status_by_file (client, file, FALSE);

    g_object_unref (file);
    g_object_unref (gfile);
    return retval;
}

static inline void set_nautilus_file_info (NautilusFileInfo *file, const char *status)
{
    if (status && strlen(status) != 0)
    {
        if (strcmp("readonly", status) == 0)
        {
            status = "unknown";
        }
        char* emblem_name = g_strconcat ("seafile-", status, NULL);
        nautilus_file_info_add_emblem (file, emblem_name);
        g_free (emblem_name);

        nautilus_file_info_add_string_attribute (file, kSeafileSyncStatus, status);
    }
}

static gboolean update_file_status_by_file (SearpcClient *client, NautilusFileInfo *file, gboolean update_file_info)
{
    char *uri = NULL;
    char* worktree;
    char* repo_id;
    GObject *repo;
    const char* path_in_repo;
    GError *error = NULL;
    char *status;
    gchar *filename;

    uri = nautilus_file_info_get_uri (file);
    filename = g_filename_from_uri (uri, NULL, NULL);
    if (!filename)
    {
        g_free (uri);
        return TRUE;
    }

    repo = find_repo_in_list (filename, watch_set_);
    if (!repo)
    {
        g_free (filename);
        g_free (uri);
        return TRUE;
    }
    g_object_get (repo, "worktree", &worktree,
                        "id", &repo_id, NULL);
    path_in_repo = filename + strlen (worktree);

    /* skip the trailing "/" */
    if (*path_in_repo == '/')
        ++path_in_repo;

    /* we can change it to a asynchronize way if we want? */
    status = searpc_client_call__string (
        client,
        "seafile_get_path_sync_status",
        &error, 3,
        "string", repo_id,
        "string", path_in_repo,
        "int", nautilus_file_info_is_directory (file));

    if (error)
    {
        gboolean is_disconnected = TRANSPORT_ERROR_CODE == error->code;
        g_error_free (error);
        g_free (worktree);
        g_free (repo_id);
        g_free (filename);
        g_free (uri);
        return !is_disconnected;
    }

    g_hash_table_replace (file_status_, filename, status);

    /* TODO refine this code */
    if (update_file_info)
    {
        set_nautilus_file_info (file, status);
    }
    else
    {
        nautilus_file_info_invalidate_extension_info (file);
    }

    g_free (worktree);
    g_free (repo_id);
    g_free (uri);
    return TRUE;
}

static gboolean update_file_status (gpointer data)
{
    SearpcClient *client;
    GHashTableIter iter;
    gpointer key, value;

    /* we handle this in a synchronize way for simplicity */

    client = seafile_rpc_get_instance ();
    g_return_val_if_fail (client, FALSE);

    g_hash_table_iter_init (&iter, file_status_);
    while (g_hash_table_iter_next (&iter, &key, &value))
    {
        g_return_val_if_fail (update_file_status_by_name (client, (const char*)key), FALSE);
    }

    return TRUE;
}

static void update_file_status_for_repo (GObject *repo, const char *status_hint, SearpcClient *client)
{
    char *worktree;
    char *repo_id;
    GFile *gfile;
    NautilusFileInfo *file;
    g_object_get (repo, "worktree", &worktree, "id", &repo_id, NULL);

    gfile = g_file_new_for_path (worktree);
    g_return_if_fail (gfile);
    file = nautilus_file_info_lookup (gfile);
    if (!file)
    {
        g_object_unref (gfile);

        g_free (repo_id);
        g_free (worktree);
        return;
    }
    set_nautilus_file_info (file, status_hint);

    g_object_unref (file);
    g_object_unref (gfile);

    g_free (repo_id);
    g_free (worktree);
}

static gboolean update_watch_set (gpointer data)
{
    GError *error = NULL;
    GList *new_watch_set;
    SearpcClient *client = seafile_rpc_get_instance ();
    (void)data; /* unused */

    g_return_val_if_fail (client, FALSE);
    g_message ("%s\n", "update watch set");

    new_watch_set = seafile_get_repo_list (client, 0, 0, &error);
    if (error != NULL)
    {
        gboolean is_disconnected = TRANSPORT_ERROR_CODE == error->code;
        g_error_free (error);
        /* call stop_update_watch_set when return */
        return !is_disconnected;
    }
    if (watch_set_)
    {
        clean_up_files_in_old_watch_set (watch_set_, new_watch_set);

        g_list_foreach (watch_set_, (GFunc)g_object_unref, NULL);
        g_list_free (watch_set_);
    }
    watch_set_ = new_watch_set;
    if (watch_set_)
    {
        GList *head = watch_set_;
        while (head)
        {
            update_file_status_for_repo (head->data, NULL, client);
            head = head->next;
        }
    }
    return TRUE;
}

static GObject* find_repo_in_list (const char* path, GList *head)
{
    g_return_val_if_fail (head, NULL);
    while (head)
    {
        char* worktree;
        int len;
        g_object_get (head->data, "worktree", &worktree, NULL);
        len = strlen(worktree);
        if (strncmp (worktree, path, len) == 0) {
            g_free (worktree);
            break;
        }
        g_free (worktree);

        head = head->next;
    }
    return head ? head->data : NULL;
}

static void clean_up_single_file (gpointer data, GObject *obj)
{
    NautilusFileInfo *file = (NautilusFileInfo*) obj;
    char *uri, *filename;
    nautilus_file_info_invalidate_extension_info (file);
    uri = nautilus_file_info_get_uri (file);
    filename = g_filename_from_uri (uri, NULL, NULL);
    if (!filename)
    {
        g_free (uri);
        return;
    }
    g_hash_table_remove (file_status_, filename);
    g_free (uri);
    g_free (filename);
}

static void clean_up_files_in_worktree(const char *worktree)
{
    GHashTableIter iter;
    gpointer key, value;

    GFile *gfile;
    NautilusFileInfo *file;
    GList *files_to_be_removed = NULL, *head = NULL;

    int worktree_len = strlen(worktree);

    g_hash_table_iter_init (&iter, file_status_);
    while (g_hash_table_iter_next (&iter, &key, &value))
    {
        if (strncmp(worktree, key, worktree_len) != 0)
        {
            continue;
        }

        files_to_be_removed = g_list_append (files_to_be_removed, g_strdup (key));

        gfile = g_file_new_for_path ((const char*)key);
        if (!gfile)
        {
            continue;
        }
        file = nautilus_file_info_lookup (gfile);
        if (!file)
        {
            g_object_unref (gfile);
            continue;
        }
        nautilus_file_info_invalidate_extension_info (file);
        g_object_unref (file);
        g_object_unref (gfile);
    }
    head = files_to_be_removed;
    while (head)
    {
        char* file_name = head->data;
        g_hash_table_remove (file_status_, file_name);
        g_free (file_name);
        head = head->next;
    }
    if (files_to_be_removed)
    {
        g_list_free (files_to_be_removed);
    }
}

static void clean_up_files_in_old_watch_set (GList *watch_set, GList *new_watch_set)
{
    char *worktree, *repo_id, *new_worktree, *new_repo_id;
    GList *head = watch_set;
    GList *new_head = new_watch_set;
    while (head)
    {
        gboolean found = FALSE;
        GObject *repo = head->data;
        g_object_get (repo, "worktree", &worktree, "id", &repo_id, NULL);
        while (new_head)
        {
            GObject *new_repo = new_head->data;
            g_object_get (new_repo, "worktree", &new_worktree, "id", &new_repo_id, NULL);
            if (strcmp (worktree, new_worktree) == 0 && strcmp (repo_id, new_repo_id) == 0)
            {
                found = TRUE;
            }
            g_free (new_repo_id);
            g_free (new_worktree);
            if (found)
            {
                break;
            }
            new_head = new_head->next;
        }
        if (!found)
        {
            clean_up_files_in_worktree (worktree);
        }
        g_free (repo_id);
        g_free (worktree);
        head = head->next;
    }
}

static void clean_up_all_data ()
{
    GHashTableIter iter;
    gpointer key, value;

    /* clean up file info */
    GFile *gfile;
    NautilusFileInfo *file;
    g_hash_table_iter_init (&iter, file_status_);
    while (g_hash_table_iter_next (&iter, &key, &value))
    {
        gfile = g_file_new_for_path ((const char*)key);
        if (!gfile)
        {
            continue;
        }
        file = nautilus_file_info_lookup (gfile);
        if (!file)
        {
            g_object_unref (gfile);
            continue;
        }
        nautilus_file_info_invalidate_extension_info (file);
        g_object_unref (file);
        g_object_unref (gfile);
    }
    g_hash_table_remove_all (file_status_);

    /* clean up watch set */
    if (watch_set_)
    {
        GList *head = watch_set_;
        while (head)
        {
            GObject *repo = head->data;
            update_file_status_for_repo (repo, "none", NULL);
            g_object_unref (head->data);
            head = head->next;
        }
        g_list_free (watch_set_);
        watch_set_ = NULL;
    }
}

