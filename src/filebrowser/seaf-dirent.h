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
    QString id;
    QString name;
    quint64 size;
    quint64 mtime;

    bool isDir() const { return type == DIR; }
    bool isFile() const { return type == FILE; }

    static SeafDirent fromJSON(const json_t*, json_error_t *error);
    static QList<SeafDirent> listFromJSON(const json_t*, json_error_t *error);
};


/**
 * Register with QMetaType so we can wrap it with QVariant::fromValue
 */
Q_DECLARE_METATYPE(SeafDirent)

#endif // SEAFILE_CLIENT_API_SEAF_DIRENT_H

