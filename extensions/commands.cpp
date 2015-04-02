#include "ext-common.h"

#include "log.h"
#include "applet-connection.h"
#include "ext-utils.h"

#include "commands.h"

namespace seafile {

uint64_t reposInfoTimestamp = 0;

GetShareLinkCommand::GetShareLinkCommand(const std::string path)
    : AppletCommand<void>("get-share-link"),
      path_(path)
{
}

std::string GetShareLinkCommand::serialize()
{
    return path_;
}

ListReposCommand::ListReposCommand()
    : AppletCommand<RepoInfoList>("list-repos")
{
}

std::string ListReposCommand::serialize()
{
    char buf[512];
    snprintf (buf, sizeof(buf), "%I64u", reposInfoTimestamp);
    return buf;
}

bool ListReposCommand::parseResponse(const std::string& raw_resp,
                                     RepoInfoList *infos)
{
    std::vector<std::string> lines = utils::split(raw_resp, '\n');
    if (lines.empty()) {
        return true;
    }
    for (size_t i = 0; i < lines.size(); i++) {
        std::string line = lines[i];
        std::vector<std::string> parts = utils::split(line, '\t');
        if (parts.size() != 4) {
            continue;
        }
        std::string repo_id, repo_name, worktree, status;
        RepoInfo::Status st;
        repo_id = parts[0];
        repo_name = parts[1];
        worktree = utils::normalizedPath(parts[2]);
        status = parts[3];
        if (status == "paused") {
            st = RepoInfo::Paused;
        } else if (status == "syncing") {
            st = RepoInfo::Syncing;
        } else if (status == "error") {
            st = RepoInfo::Error;
        } else if (status == "normal") {
            st = RepoInfo::Normal;
        } else {
            // impossible
            seaf_ext_log ("bad repo status \"%s\"", status.c_str());
            continue;
        }
        // seaf_ext_log ("status for %s is \"%s\"", repo_name.c_str(), status.c_str());
        infos->push_back(RepoInfo(repo_id, repo_name, worktree, st));
    }

    reposInfoTimestamp = utils::currentMSecsSinceEpoch();
    return true;

}

} // namespace seafile
