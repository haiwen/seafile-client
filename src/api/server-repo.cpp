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
    repo.owner = getStringFromJson(json, "owner");
    repo.permission = getStringFromJson(json, "permission");

    repo._virtual = json_is_true(json_object_get(json, "virtual"));

    if (repo.type == "grepo") {
        repo.group_name = repo.owner;
        repo.group_id = json_integer_value(json_object_get(json, "groupid"));
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
    if (encrypted) {
        return QIcon(":/images/encrypted-repo.png");
    } else {
        return QIcon(":/images/repo.png");
    }
}

QPixmap ServerRepo::getPixmap() const
{
    if (encrypted) {
        return QPixmap(":/images/encrypted-repo.png");
    } else {
        return QPixmap(":/images/repo.png");
    }
}
