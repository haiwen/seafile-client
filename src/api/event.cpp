#include <jansson.h>

#include <vector>

#include "utils/translate-commit-desc.h"


#include "event.h"

#include <QDateTime>

namespace {

QString getStringFromJson(const json_t *json, const char* key)
{
    return QString::fromUtf8(json_string_value(json_object_get(json, key)));
}

const char *kEventTypeRepoCreate = "repo-create";
const char *kEventTypeRepoDelete = "repo-delete";

} // namespace


SeafEvent SeafEvent::fromJSON(const json_t *json, json_error_t */* error */)
{
    SeafEvent event;

    event.author = getStringFromJson(json, "author");
    if (event.author.isEmpty()) {
        event.author = "anonymous";
        event.anonymous = true;
    } else {
        event.anonymous = false;
    }

    event.nick = getStringFromJson(json, "nick");
    if (event.nick.isEmpty()) {
        event.nick = "anonymous";
    }

    event.repo_id = getStringFromJson(json, "repo_id");
    event.repo_name = getStringFromJson(json, "repo_name");

    event.commit_id = getStringFromJson(json, "commit_id");
    event.etype = getStringFromJson(json, "etype");
    event.desc = getStringFromJson(json, "desc");

    event.timestamp = json_integer_value(json_object_get(json, "time"));

    if (event.etype == kEventTypeRepoCreate) {
        event.desc = QObject::tr("Created library \"%1\"").arg(event.repo_name);
    } else if (event.etype == kEventTypeRepoDelete) {
        event.desc = QObject::tr("Deleted library \"%1\"").arg(event.repo_name);
    }

    event.desc = translateCommitDesc(event.desc);

    // For testing long lines of event description.
    // event.desc += event.desc + event.desc;

    return event;
}

// parser seafile new file activities json
SeafEvent SeafEvent::fromJSONV2(const json_t *json, json_error_t */* error */)
{
    SeafEvent event;

    event.author = getStringFromJson(json, "author_email");
    if (event.author.isEmpty()) {
        event.author = "anonymous";
        event.anonymous = true;
    } else {
        event.anonymous = false;
    }

    event.nick = getStringFromJson(json, "author_name");
    if (event.nick.isEmpty()) {
        event.nick = "anonymous";
    }

    event.repo_id = getStringFromJson(json, "repo_id");
    event.repo_name = getStringFromJson(json, "repo_name");
    event.commit_id = getStringFromJson(json, "commit_id");
    event.etype = getStringFromJson(json, "op_type");

    QString time = getStringFromJson(json, "time");
    event.timestamp = QDateTime::fromString(time, Qt::ISODate).toMSecsSinceEpoch()/1000;

    QString path = getStringFromJson(json, "path");
    QString file_name = getStringFromJson(json, "name");
    QString repo_name = event.repo_name;
    QString obj_type = getStringFromJson(json, "obj_type");
    QString op_type = event.etype;

    event.desc = translateCommitDescV2(path, file_name, repo_name, obj_type, op_type);

    return event;
}

std::vector<SeafEvent> SeafEvent::listFromJSON(const json_t *json, json_error_t *error, bool is_use_new_json_parsor)
{
    std::vector<SeafEvent> events;

    for (size_t i = 0; i < json_array_size(json); i++) {
        SeafEvent event;
        if (!is_use_new_json_parsor) {
            event = fromJSON(json_array_get(json, i), error);
        } else {
            event = fromJSONV2(json_array_get(json, i), error);
        }

        events.push_back(event);
    }

    return events;
}

QString SeafEvent::toString() const
{
    return QString("type=\"%1\",author=\"%2\",repo_name=\"%3\",desc=\"%4\",commit=\"%5\"")
        .arg(etype).arg(author).arg(repo_name).arg(desc).arg(commit_id);
}

bool SeafEvent::isDetailsDisplayable() const
{
    if (commit_id.isEmpty()) {
        return false;
    }
    // TODO: determine if there are files change in this commit
    return true;
}
