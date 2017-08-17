#ifndef SEAFILE_CLIENT_CONTACT_SHARE_INFO_H
#define SEAFILE_CLIENT_CONTACT_SHARE_INFO_H

#include <QMetaType>
#include <QString>

struct SeafileGroup {
    int id;
    QString name;
    QString owner;
    bool isValid() const { return !name.isEmpty(); }
};

struct SeafileUser {
    QString avatar_url;
    QString email;
    // Optional fields;
    QString contact_email;
    QString name;

    bool operator==(const SeafileUser& rhs) const {
        return rhs.email == email;
    }

    QString getDisplayEmail() const {
        return !contact_email.isEmpty() ? contact_email : email;
    }

    bool isValid() const { return !email.isEmpty(); }
};

uint qHash(const SeafileUser& user, uint seed=0);

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
    SeafileUser user;
};

struct GroupShareInfo {
    SharePermission permission;
    SeafileGroup group;
};

/**
 * Register with QMetaType so we can wrap it with QVariant::fromValue
 */
Q_DECLARE_METATYPE(SeafileGroup)
Q_DECLARE_METATYPE(SeafileUser)
Q_DECLARE_METATYPE(UserShareInfo)
Q_DECLARE_METATYPE(GroupShareInfo)

#endif // SEAFILE_CLIENT_CONTACT_SHARE_INFO_H
