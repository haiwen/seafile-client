#include "commit-details.h"

namespace {

QString getStringFromJsonArray(const json_t *array, size_t index)
{
    return QString::fromUtf8(json_string_value(json_array_get(array, index)));
}

void processFileList(const json_t *json, const char *key, std::vector<QString> *files)
{
    json_t *array = json_object_get(json, key);
    if (array) {
        for (int i = 0, n = json_array_size(array); i < n; i++) {
            QString name = getStringFromJsonArray(array, i);
            files->push_back(name);
        }
    }
}

} // namespace


CommitDetails CommitDetails::fromJSON(const json_t *json, json_error_t */* error */)
{
    CommitDetails details;

    processFileList(json, "added_files", &details.added_files);
    processFileList(json, "deleted_files", &details.deleted_files);
    processFileList(json, "modified_files", &details.modified_files);

    processFileList(json, "added_dirs", &details.added_dirs);
    processFileList(json, "deleted_files", &details.deleted_files);

    return details;
}
