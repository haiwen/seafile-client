#ifndef SEAFILE_CLIENT_LOCAL_REPO_H
#define SEAFILE_CLIENT_LOCAL_REPO_H

/**
 * Repo Information from local seaf-daemon
 */
class LocalRepo {
public:
    QString id;
    QString name;
    QString description;

    qint64 mtime;
    bool encrypted;

    qint64  size;
    QString root;
};

#endif // SEAFILE_CLIENT_LOCAL_REPO_H
