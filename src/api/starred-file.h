#ifndef SEAFILE_CLIENT_STARRED_FILE_H
#define SEAFILE_CLIENT_STARRED_FILE_H

#include <vector>
#include <QString>
#include <QMetaType>
#include <jansson.h>

class StarredFile {
public:
    QString repo_id;
    QString repo_name;
    QString path;
    qint64 size;
    qint64 mtime;

    static StarredFile fromJSON(const json_t*, json_error_t *error);
    static std::vector<StarredFile> listFromJSON(const json_t*, json_error_t *json);

    const QString name() const;
};

/**
 * Register with QMetaType so we can wrap it with QVariant::fromValue
 */
Q_DECLARE_METATYPE(StarredFile)

#endif // SEAFILE_CLIENT_STARRED_FILE_H
