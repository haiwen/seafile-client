#include <vector>
#include <jansson.h>

#include "server-repo.h"
#include "requests.h"

namespace {

QString getStringFromJson(const json_t *json, const char* key)
{
    return QString::fromUtf8(json_string_value(json_object_get(json, key)));
}

} // namespace

ServerRepo:: ~ServerRepo()
{
    if (req)
        delete req;
}

ServerRepo ServerRepo::fromJSON(const json_t *json, json_error_t *error)
{
    ServerRepo repo;
    repo.id = getStringFromJson(json, "id");
    repo.name = getStringFromJson(json, "name");
    repo.description = getStringFromJson(json, "desc");
    repo.owner = getStringFromJson(json, "owner");
    repo.permission = getStringFromJson(json, "permission");
    repo.mtime = json_integer_value(json_object_get(json, "mtime"));
    repo.encrypted = json_is_true(json_object_get(json, "encrypted"));
    repo.root = getStringFromJson(json, "root");
    repo.size = json_integer_value(json_object_get(json, "size"));
    repo.is_group_repo = getStringFromJson(json, "type") == "type";

    return repo;
}

std::vector<ServerRepo> ServerRepo::listFromJSON(const json_t *json, json_error_t *error)
{
    std::vector<ServerRepo> repos;
    for (int i = 0; i < json_array_size(json); i++) {
        ServerRepo repo = fromJSON(json_array_get(json, i), error);
        repos.push_back(repo);
    }

    return repos;
}

