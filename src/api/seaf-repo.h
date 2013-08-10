#ifndef SEAFILE_CLIENT_SEAF_REPO_H
#define SEAFILE_CLIENT_SEAF_REPO_H

#include <vector>
#include <QString>
#include <jansson.h>


struct SeafRepo {
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

    static SeafRepo fromJSON(const json_t*, json_error_t *error);
    static std::vector<SeafRepo> listFromJSON(const json_t*, json_error_t *json);
};

#endif
