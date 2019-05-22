#ifndef SEAFILE_CLIENT_API_EVENT_H
#define SEAFILE_CLIENT_API_EVENT_H

#include <jansson.h>
#include <vector>

#include <QObject>
#include <QString>
#include <QMetaType>

class SeafEvent {
public:
    QString author;
    QString nick;
    QString repo_id;
    QString repo_name;
    QString etype;
    QString commit_id;
    QString desc;
    QString op_desc;
    qint64 timestamp;
    bool is_use_new_activities_api;

    // true for events like a file upload by unregistered user from a
    // uploadable link
    bool anonymous;

    bool isDetailsDisplayable() const;
    
    static SeafEvent fromJSON(const json_t*, json_error_t *error);
    static SeafEvent fromJSONV2(const json_t*, json_error_t *error);
    static std::vector<SeafEvent> listFromJSON(const json_t*, json_error_t *json, bool is_use_new_json_parser = false);

    QString toString() const;
};

/**
 * Register with QMetaType so we can wrap it with QVariant::fromValue
 */
Q_DECLARE_METATYPE(SeafEvent)

#endif // SEAFILE_CLIENT_API_EVENT_H
