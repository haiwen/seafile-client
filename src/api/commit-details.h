#ifndef SEAFILE_CLIENT_API_COMMIT_DETAILS_H
#define SEAFILE_CLIENT_API_COMMIT_DETAILS_H

#include <jansson.h>

#include <vector>
#include <utility>

#include <QString>
#include <QMetaType>

struct CommitDetails {
    std::vector<QString> added_files, deleted_files, modified_files, added_dirs, deleted_dirs;

    // renamed or moved files
    std::vector<std::pair<QString, QString> > renamed_files;

    static CommitDetails fromJSON(const json_t*, json_error_t *error);
};


/**
 * Register with QMetaType so we can wrap it with QVariant::fromValue
 */
Q_DECLARE_METATYPE(CommitDetails)

#endif // SEAFILE_CLIENT_API_COMMIT_DETAILS_H
