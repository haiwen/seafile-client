#ifndef SEAFILE_CLIENT_CONTACT_SHARE_INFO_H
#define SEAFILE_CLIENT_CONTACT_SHARE_INFO_H

#include <QMetaType>
#include <QString>

struct SeafileGroup {
    int id;
    QString name;
    QString owner;
};

struct SeafileContact {
    QString email;
    QString name;
};

enum SharePermission {
    READ_WRITE,
    READ_ONLY,
};


enum ShareType {
    SHARE_TO_USER,
    SHARE_TO_GROUP,
};


inline SharePermission permissionfromString(const QString& s)
{
    return s == "r" ? READ_ONLY : READ_WRITE;
}

inline ShareType shareTypeFromString(const QString& s)
{
    return s == "group" ? SHARE_TO_GROUP : SHARE_TO_USER;
}

struct UserShareInfo {
    SharePermission permission;
    SeafileContact user;
};

struct GroupShareInfo {
    SharePermission permission;
    SeafileGroup group;
};

/**
 * Register with QMetaType so we can wrap it with QVariant::fromValue
 */
Q_DECLARE_METATYPE(SeafileGroup)
Q_DECLARE_METATYPE(SeafileContact)
Q_DECLARE_METATYPE(UserShareInfo)
Q_DECLARE_METATYPE(GroupShareInfo)

#endif
