#ifndef SEAFILE_CLIENT_SERVER_REPO_H
#define SEAFILE_CLIENT_SERVER_REPO_H

#include <vector>
#include <QString>
#include <QMetaType>
#include <QIcon>
#include <QPixmap>
#include <jansson.h>

/**
 * Repo information from seahub api
 */
class ServerRepo {
public:
    ServerRepo(){};

    QString id;
    QString name;
    QString description;

    qint64 mtime;
    qint64  size;
    QString root;

    bool encrypted;

    // "virtual" is a reserved word in C++
    bool _virtual;

    QString type;
    QString owner;
    QString permission;
    QString group_name;
    int group_id;

    bool isPersonalRepo() const { return type == "repo"; }
    bool isSharedRepo() const { return type == "srepo"; }
    bool isGroupRepo() const { return type == "grepo"; }

    bool isVirtual() const { return _virtual; }

    QIcon getIcon() const;
    QPixmap getPixmap() const;

    static ServerRepo fromJSON(const json_t*, json_error_t *error);
    static std::vector<ServerRepo> listFromJSON(const json_t*, json_error_t *json);
};


/**
 * Register with QMetaType so we can wrap it with QVariant::fromValue
 */
Q_DECLARE_METATYPE(ServerRepo)


#endif // SEAFILE_CLIENT_SERVER_REPO_H
