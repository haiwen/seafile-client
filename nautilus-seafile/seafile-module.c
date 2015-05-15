#include <stdio.h>
#include "nautilus-seafile.h"

static GType type_list[1];

void nautilus_module_initialize(GTypeModule *module)
{
    g_message ("Initializing nautilus-seafile extension\n", NULL);
    seafile_extension_register_type (module);
    type_list[0] = SEAFILE_TYPE_EXTENSION;
}

void nautilus_module_shutdown(void)
{
    g_message ("Unloading nautilus-seafile extension\n", NULL);
}

void nautilus_module_list_types(const GType **types, int *num_types) {
    *types = type_list;
    *num_types = G_N_ELEMENTS(type_list);
}
