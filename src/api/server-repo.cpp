#include <vector>
#include <jansson.h>
#include <QPixmap>

#include "server-repo.h"

namespace {

QString getStringFromJson(const json_t *json, const char* key)
{
    return QString::fromUtf8(json_string_value(json_object_get(json, key)));
}

} // namespace


ServerRepo ServerRepo::fromJSON(const json_t *json, json_error_t */* error */)
{
    ServerRepo repo;
    repo.id = getStringFromJson(json, "id");
    repo.name = getStringFromJson(json, "name");
    repo.description = getStringFromJson(json, "desc");

    repo.mtime = json_integer_value(json_object_get(json, "mtime"));
    repo.size = json_integer_value(json_object_get(json, "size"));
    repo.root = getStringFromJson(json, "root");

    repo.encrypted = json_is_true(json_object_get(json, "encrypted"));

    repo.type = getStringFromJson(json, "type");
    repo.permission = getStringFromJson(json, "permission");
    repo.readonly = (repo.permission == "r") ? true : false;

    repo._virtual = json_is_true(json_object_get(json, "virtual"));

    if (repo.type == "grepo") {
        repo.owner = getStringFromJson(json, "share_from");
        repo.group_name = getStringFromJson(json, "owner");
        repo.group_id = json_integer_value(json_object_get(json, "groupid"));
    } else {
        repo.owner = getStringFromJson(json, "owner");
        repo.group_name = QString();
        repo.group_id = 0;
    }

    return repo;
}

std::vector<ServerRepo> ServerRepo::listFromJSON(const json_t *json, json_error_t *error)
{
    std::vector<ServerRepo> repos;
    for (size_t i = 0; i < json_array_size(json); i++) {
        ServerRepo repo = fromJSON(json_array_get(json, i), error);
        repos.push_back(repo);
    }

    return repos;
}

QIcon ServerRepo::getIcon() const
{
    if (this->isSubfolder()) {
        return QIcon(":/images/main-panel/folder.png");
    } else if (encrypted) {
        return QIcon(":/images/main-panel/library-encrypted.png");
    } else if (readonly) {
        return QIcon(":/images/main-panel/library-readonly.png");
    } else {
        return QIcon(":/images/main-panel/library-normal.png");
    }
}

QPixmap ServerRepo::getPixmap(int size) const
{
    return getIcon().pixmap(size);
}
