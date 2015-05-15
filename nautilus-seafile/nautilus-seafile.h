#ifndef H_NAUTILUS_SEAFILE_H
#define H_NAUTILUS_SEAFILE_H
#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define SEAFILE_TYPE_EXTENSION         (seafile_extension_get_type ())
#define SEAFILE_EXTENSION(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), SEAFILE_TYPE_EXTENSION, SeafileExtension))
#define SEAFILE_IS_EXTENSION(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), SEAFILE_TYPE_EXTENSION))

typedef struct _SeafileExtension       SeafileExtension;
typedef struct _SeafileExtensionClass  SeafileExtensionClass;

struct _SeafileExtension {
    GObject parent_slot;
};

struct _SeafileExtensionClass {
    GObjectClass parent_slot;
};

GType seafile_extension_get_type      (void);
void  seafile_extension_register_type (GTypeModule *module);

G_END_DECLS


#endif /* H_NAUTILUS_SEAFILE_H */
