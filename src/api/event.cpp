#include <jansson.h>

#include <vector>

#include "utils/translate-commit-desc.h"


#include "event.h"

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

std::vector<SeafEvent> SeafEvent::listFromJSON(const json_t *json, json_error_t *error)
{
    std::vector<SeafEvent> events;

    for (size_t i = 0; i < json_array_size(json); i++) {
        SeafEvent event = fromJSON(json_array_get(json, i), error);
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
