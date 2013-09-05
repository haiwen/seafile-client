#ifndef SEAFILE_CLIENT_SERVER_REPO_H
#define SEAFILE_CLIENT_SERVER_REPO_H

#include <vector>
#include <QString>
#include <QMetaType>
#include <QIcon>
#include <jansson.h>

/**
 * Repo information from seahub api
 */
class ServerRepo {
public:
    QString id;
    QString name;
    QString description;
    QString owner;

    qint64 mtime;
    bool is_group_repo;
    bool encrypted;
    QString permission;

    qint64  size;
    QString root;

    bool download; // download or sync with local dir
    QString passwd;
    QString localdir;

    QIcon getIcon();

    static ServerRepo fromJSON(const json_t*, json_error_t *error);
    static std::vector<ServerRepo> listFromJSON(const json_t*, json_error_t *json);
};

Q_DECLARE_METATYPE(ServerRepo)

#endif // SEAFILE_CLIENT_SERVER_REPO_H
