#ifndef SEAFILE_CLIENT_SEAF_REPO_H
#define SEAFILE_CLIENT_SEAF_REPO_H

#include <QString>

struct SeafRepo {
    QString id;
    QString name;
    QString description;
    QString owner;

    qint64 mtime;
    bool is_group_repo;
    bool encrypted;
    bool permission;

    qint64  size;
    QString root;
};

#endif
