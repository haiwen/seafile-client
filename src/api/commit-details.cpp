#include <glib.h>
#include <glib-object.h>

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
    processFileList(json, "deleted_dirs", &details.deleted_dirs);

    // process renamed files
    json_t *array = json_object_get(json, "renamed_files");
    if (array) {
        for (int i = 0, n = json_array_size(array); i < n; i += 2) {
            QString before_rename = getStringFromJsonArray(array, i);
            QString after_rename = getStringFromJsonArray(array, i + 1);
            std::pair<QString, QString> pair(before_rename, after_rename);
            details.renamed_files.push_back(pair);
        }
    }

    return details;
}


CommitDetails CommitDetails::fromObjList(const GList *objlist)
{
    CommitDetails details;

    for (const GList *ptr = objlist; ptr; ptr = ptr->next) {
        GObject *obj = (GObject *)(ptr->data);
        char *c_status = NULL;
        char *c_name = NULL;
        char *c_new_name = NULL;
        g_object_get(obj,
                     "status",
                     &c_status,
                     "name",
                     &c_name,
                     "new_name",
                     &c_new_name,
                     NULL);

        QString status(c_status);
        QString name(c_name);
        QString new_name(c_new_name);

        // printf("%s\n%s\n%s\n",
        //        status.toUtf8().data(),
        //        name.toUtf8().data(),
        //        new_name.toUtf8().data());

        if (status == "add") {
            details.added_files.push_back(name);
        } else if (status == "del") {
            details.deleted_files.push_back(name);
        } else if (status == "mov") {
            details.renamed_files.push_back({name, new_name});
        } else if (status == "mod") {
            details.modified_files.push_back(name);
        } else if (status == "newdir") {
            details.added_dirs.push_back(name);
        } else if (status == "deldir") {
            details.deleted_dirs.push_back(name);
        }
    }

    return details;
}
