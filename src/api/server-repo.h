#ifndef SEAFILE_CLIENT_SEAF_REPO_H
#define SEAFILE_CLIENT_SEAF_REPO_H

#include <vector>
#include <QString>
#include <jansson.h>

class DownloadRepoRequest;

/**
 * Repo information from seahub api
 */
class ServerRepo {
public:
    ServerRepo():req(NULL), passwd(QString::null){};
    ~ServerRepo();

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
    DownloadRepoRequest *req;

    static ServerRepo fromJSON(const json_t*, json_error_t *error);
    static std::vector<ServerRepo> listFromJSON(const json_t*, json_error_t *json);
};

#endif
