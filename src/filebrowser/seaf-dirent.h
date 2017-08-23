#ifndef SEAFILE_CLIENT_API_SEAF_DIRENT_H
#define SEAFILE_CLIENT_API_SEAF_DIRENT_H

#include <jansson.h>

#include <vector>
#include <QString>
#include <QList>
#include <QMetaType>

class SeafDirent {
public:
    enum Type {
        DIR,
        FILE
    };
    
    Type type;
    bool readonly;
    QString id;
    QString name;
    quint64 size;
    quint64 mtime;

    bool is_locked;
    bool locked_by_me;
    QString lock_owner;
    QString lock_owner_name;
    quint64 lock_time;

    bool isDir() const { return type == DIR; }
    bool isFile() const { return type == FILE; }

    const QString& getLockOwnerDisplayString() const;

    static SeafDirent fromJSON(const json_t*, json_error_t *error);
    static QList<SeafDirent> listFromJSON(const json_t*, json_error_t *error);

    static SeafDirent dir(const QString& name);
    static SeafDirent file(const QString& name, quint64 size);
};


/**
 * Register with QMetaType so we can wrap it with QVariant::fromValue
 */
Q_DECLARE_METATYPE(SeafDirent)

#endif // SEAFILE_CLIENT_API_SEAF_DIRENT_H

