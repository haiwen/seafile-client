#include <vector>
#include <jansson.h>

#include "seaf-repo.h"

namespace {

QString getStringFromJson(const json_t *json, const char* key)
{
    return QString::fromUtf8(json_string_value(json_object_get(json, key)));
}

} // namespace

SeafRepo SeafRepo::fromJSON(const json_t *json, json_error_t *error)
{
    SeafRepo repo;
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

std::vector<SeafRepo> SeafRepo::listFromJSON(const json_t *json, json_error_t *error)
{
    std::vector<SeafRepo> repos;
    for (int i = 0; i < json_array_size(json); i++) {
        SeafRepo repo = fromJSON(json_array_get(json, i), error);
        repos.push_back(repo);
    }

    return repos;
}
