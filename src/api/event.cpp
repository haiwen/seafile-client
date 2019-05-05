#include <jansson.h>

#include <vector>

#include "utils/translate-commit-desc.h"


#include "seafile-applet.h"
#include "account-mgr.h"
#include "event.h"
#include "utils/utils.h"

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
    const Account account = seafApplet->accountManager()->currentAccount();
    // if server version is least version 7.0.0 enable new api
    bool is_support_new_file_activities_api = false;
    if (account.isValid()) {
        is_support_new_file_activities_api = account.isAtLeastVersion(6, 3, 1);
    }

    if (!is_support_new_file_activities_api) {
        event.author = getStringFromJson(json, "author");
    } else {
        event.author = getStringFromJson(json, "author_email");
    }

    if (event.author.isEmpty()) {
        event.author = "anonymous";
        event.anonymous = true;
    } else {
        event.anonymous = false;
    }

    if (!is_support_new_file_activities_api) {
        event.nick = getStringFromJson(json, "nick");
    } else {
        event.nick = getStringFromJson(json, "author_name");
    }

    if (event.nick.isEmpty()) {
        event.nick = "anonymous";
    }

    event.repo_id = getStringFromJson(json, "repo_id");
    event.repo_name = getStringFromJson(json, "repo_name");

    event.commit_id = getStringFromJson(json, "commit_id");

    if (!is_support_new_file_activities_api) {
        event.etype = getStringFromJson(json, "etype");
    } else {
        event.etype = getStringFromJson(json, "op_type");
    }

    if (!is_support_new_file_activities_api) {
        event.desc = getStringFromJson(json, "desc");
    } else {
        event.desc = QString("");
    }

    if (!is_support_new_file_activities_api) {
        event.timestamp = json_integer_value(json_object_get(json, "time"));
    } else {
        QString time = getStringFromJson(json, "time");
        QString str_date = time.mid(0, 10).replace("-", "");
        QString str_time = time.mid(11, 8).replace(":", "");
        QString str_date_time = str_date + str_time;

        event.timestamp = transferToTimestamp(str_date_time);

    }

    if (!is_support_new_file_activities_api) {
        if (event.etype == kEventTypeRepoCreate) {
            event.desc = QObject::tr("Created library \"%1\"").arg(event.repo_name);
        } else if (event.etype == kEventTypeRepoDelete) {
            event.desc = QObject::tr("Deleted library \"%1\"").arg(event.repo_name);
        }
        event.desc = translateCommitDesc(event.desc);
    } else {
        QString path = getStringFromJson(json, "path");
        QString file_name = getStringFromJson(json, "name");
        QString repo_name = event.repo_name;
        QString obj_type = getStringFromJson(json, "obj_type");
        QString op_type = event.etype;

        event.desc = translateComitActivitiesDesc(path, file_name, repo_name, obj_type, op_type);

    }


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
