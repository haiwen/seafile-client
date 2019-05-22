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
    ServerRepo() : encrypted(false), readonly(false), _virtual(false), group_id (0) {};

    QString id;
    QString name;
    QString description;

    qint64 mtime;
    qint64  size;
    QString root;

    bool encrypted;
    bool readonly;

    // "virtual" is a reserved word in C++
    bool _virtual;

    // subfolder attributes
    QString parent_repo_id;
    QString parent_path;

    QString type;
    QString owner;
    QString permission;
    QString group_name;
    int group_id;

    bool isValid() const { return !id.isEmpty(); }

    bool isPersonalRepo() const { return type == "repo"; }
    bool isSharedRepo() const { return type == "srepo"; }
    bool isGroupRepo() const { return type == "grepo"; }
    bool isOrgRepo() const { return isGroupRepo() and group_id == 0; }

    bool isVirtual() const { return _virtual; }
    bool isSubfolder() const { return !parent_repo_id.isEmpty() && !parent_path.isEmpty(); }

    QIcon getIcon() const;
    QPixmap getPixmap(int size = 24) const;

    static ServerRepo fromJSON(const json_t*, json_error_t *error);
    static std::vector<ServerRepo> listFromJSON(const json_t*, json_error_t *json);
};


/**
 * Register with QMetaType so we can wrap it with QVariant::fromValue
 */
Q_DECLARE_METATYPE(ServerRepo)


#endif // SEAFILE_CLIENT_SERVER_REPO_H
