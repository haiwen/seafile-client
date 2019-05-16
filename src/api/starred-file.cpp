#include <jansson.h>

#include <vector>
#include <QFileInfo>
#include <QDateTime>

#include "starred-file.h"

namespace {

QString getStringFromJson(const json_t *json, const char* key)
{
    return QString::fromUtf8(json_string_value(json_object_get(json, key)));
}

} // namespace


StarredItem StarredItem::fromJSON(const json_t *json, json_error_t */* error */)
{
    StarredItem file;
    file.repo_id = getStringFromJson(json, "repo_id");
    if (file.repo_id.isEmpty()) {
        file.repo_id = getStringFromJson(json, "repo");
    }
    file.repo_name = getStringFromJson(json, "repo_name");
    file.path = getStringFromJson(json, "path");

    file.mtime = json_integer_value(json_object_get(json, "mtime"));
    file.size = json_integer_value(json_object_get(json, "size"));

    return file;
}

StarredItem StarredItem::fromJSONV2(const json_t *json, json_error_t */* error */)
{
    StarredItem file;
    file.repo_id = getStringFromJson(json, "repo_id");

    file.repo_name = getStringFromJson(json, "repo_name");
    file.path = getStringFromJson(json, "path");
    file.type = StarredItem::FILE;
    if (file.path == "/") {
        file.type = StarredItem::REPO;
    } else if ((file.path.size() > 1) && (file.path.endsWith("/"))) {
        file.type = StarredItem::DIR;
    }

    file.obj_name = getStringFromJson(json, "obj_name");

    QString date_time = getStringFromJson(json, "mtime");
    file.mtime = QDateTime::fromString(date_time, Qt::ISODate).toMSecsSinceEpoch() / 1000;

    file.from_new_api = true;

    return file;
}

std::vector<StarredItem> StarredItem::listFromJSON(const json_t *json, json_error_t *error, bool is_use_new_json_parser)
{
    std::vector<StarredItem> files;
    for (size_t i = 0; i < json_array_size(json); i++) {
        StarredItem file;
        if (is_use_new_json_parser) {
            file = fromJSONV2(json_array_get(json, i), error);
        } else {
            file = fromJSON(json_array_get(json, i), error);
        }
        files.push_back(file);
    }

    return files;
}

const QString StarredItem::name() const
{
    if (from_new_api) {
        return obj_name;
    } else {
        return QFileInfo(path).fileName();
    }
}

bool StarredItem::isFile() const
{
    return type == FILE;
}
