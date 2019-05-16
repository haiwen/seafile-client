#ifndef SEAFILE_CLIENT_STARRED_FILE_H
#define SEAFILE_CLIENT_STARRED_FILE_H

#include <vector>
#include <QString>
#include <QMetaType>
#include <jansson.h>

class StarredItem {
public:
    QString repo_id;
    QString repo_name;
    QString path;
    qint64 size;
    qint64 mtime;
    QString obj_name;

    enum StarredItemType {
        FILE = 0,
        DIR,
        REPO
    } type;

    bool from_new_api;

    static StarredItem fromJSON(const json_t*, json_error_t *error);
    static StarredItem fromJSONV2(const json_t*, json_error_t *error);
    static std::vector<StarredItem> listFromJSON(const json_t*, json_error_t *json, bool is_use_new_json_parser = false);

    const QString name() const;
    bool isFile() const;
};

/**
 * Register with QMetaType so we can wrap it with QVariant::fromValue
 */
Q_DECLARE_METATYPE(StarredItem)

#endif // SEAFILE_CLIENT_STARRED_FILE_H
