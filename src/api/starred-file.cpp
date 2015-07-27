#include <jansson.h>

#include <vector>
#include <QFileInfo>

#include "starred-file.h"

namespace {

QString getStringFromJson(const json_t *json, const char* key)
{
    return QString::fromUtf8(json_string_value(json_object_get(json, key)));
}

} // namespace


StarredFile StarredFile::fromJSON(const json_t *json, json_error_t */* error */)
{
    StarredFile file;
    file.repo_id = getStringFromJson(json, "repo_id");
    file.repo_name = getStringFromJson(json, "repo_name");
    file.path = getStringFromJson(json, "path");

    file.mtime = json_integer_value(json_object_get(json, "mtime"));
    file.size = json_integer_value(json_object_get(json, "size"));

    return file;
}

std::vector<StarredFile> StarredFile::listFromJSON(const json_t *json, json_error_t *error)
{
    std::vector<StarredFile> files;
    for (size_t i = 0; i < json_array_size(json); i++) {
        StarredFile file = fromJSON(json_array_get(json, i), error);
        files.push_back(file);
    }

    return files;
}

const QString StarredFile::name() const
{
    return QFileInfo(path).fileName();
}

