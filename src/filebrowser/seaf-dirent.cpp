#include <jansson.h>

#include "utils/json-utils.h"

#include "seaf-dirent.h"

SeafDirent SeafDirent::fromJSON(const json_t *root, json_error_t */* error */)
{
    SeafDirent dirent;
    Json json(root);

    dirent.id = json.getString("id");
    dirent.name = json.getString("name");

    QString type = json.getString("type");
    if (type == "file") {
        dirent.type = FILE;
        dirent.size = json.getLong("size");
    } else {
        dirent.type = DIR;
    }
    dirent.mtime = json.getLong("mtime");

    return dirent;
}

QList<SeafDirent> SeafDirent::listFromJSON(const json_t *json, json_error_t *error)
{
    QList<SeafDirent> dirents;
    for (size_t i = 0; i < json_array_size(json); i++) {
        SeafDirent dirent = fromJSON(json_array_get(json, i), error);
        dirents.push_back(dirent);
    }

    return dirents;
}
