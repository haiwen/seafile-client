#include <jansson.h>
#include <QDateTime>

#include "utils/json-utils.h"

#include "seaf-dirent.h"

namespace {

void initCommonFields(SeafDirent *dirent) {
    dirent->mtime = QDateTime::currentDateTime().toTime_t();
    dirent->readonly = false;
    dirent->is_locked = false;
    dirent->locked_by_me = false;
}

}

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
    dirent.readonly = json.getString("permission") == "r" ? true : false;
    dirent.mtime = json.getLong("mtime");

    dirent.is_locked = json.getBool("is_locked");
    dirent.lock_owner = json.getString("lock_owner");
    dirent.lock_owner_name = json.getString("lock_owner_name");
    dirent.lock_time = json.getLong("lock_time");
    dirent.locked_by_me = json.getBool("locked_by_me");

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

const QString& SeafDirent::getLockOwnerDisplayString() const
{
    return !lock_owner_name.isEmpty() ? lock_owner_name : lock_owner;
}

SeafDirent SeafDirent::dir(const QString& name)
{
    SeafDirent dirent;
    dirent.type = DIR;
    dirent.name = name;

    initCommonFields(&dirent);
    return dirent;
}

SeafDirent SeafDirent::file(const QString& name, quint64 size)
{
    SeafDirent dirent;
    dirent.type = FILE;
    dirent.name = name;
    dirent.size = size;

    initCommonFields(&dirent);
    return dirent;
}
