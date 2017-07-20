#include "ext-common.h"

#include "log.h"
#include "applet-connection.h"
#include "ext-utils.h"

#include "commands.h"

namespace seafile {

uint64_t reposInfoTimestamp = 0;

std::string toString(RepoInfo::Status st) {
    switch (st) {
    case RepoInfo::NoStatus:
        return "nostatus";
    case RepoInfo::Paused:
        return "paused";
    case RepoInfo::Normal:
        return "synced";
    case RepoInfo::Syncing:
        return "syncing";
    case RepoInfo::Error:
        return "error";
    case RepoInfo::ReadOnly:
        return "readonly";
    case RepoInfo::LockedByMe:
        return "locked by me";
    case RepoInfo::LockedByOthers:
        return "locked by someone else";
    case RepoInfo::N_Status:
        return "";
    }
    return "";
}


GetShareLinkCommand::GetShareLinkCommand(const std::string path)
    : AppletCommand<void>("get-share-link"),
      path_(path)
{
}

std::string GetShareLinkCommand::serialize()
{
    return path_;
}

GetInternalLinkCommand::GetInternalLinkCommand(const std::string path)
    : AppletCommand<void>("get-internal-link"),
      path_(path)
{
}

std::string GetInternalLinkCommand::serialize()
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
                                     RepoInfoList* infos)
{
    std::vector<std::string> lines = utils::split(raw_resp, '\n');
    if (lines.empty()) {
        return true;
    }
    for (size_t i = 0; i < lines.size(); i++) {
        std::string line = lines[i];
        std::vector<std::string> parts = utils::split(line, '\t');
        if (parts.size() != 6) {
            continue;
        }
        std::string repo_id, repo_name, worktree, status;
        RepoInfo::Status st;
        bool support_file_lock;
        bool support_private_share;

        repo_id = parts[0];
        repo_name = parts[1];
        worktree = utils::normalizedPath(parts[2]);
        status = parts[3];
        support_file_lock = parts[4] == "file-lock-supported";
        support_private_share = parts[5] == "private-share-supported";
        if (status == "paused") {
            st = RepoInfo::Paused;
        }
        else if (status == "syncing") {
            st = RepoInfo::Syncing;
        }
        else if (status == "error") {
            st = RepoInfo::Error;
        }
        else if (status == "normal") {
            st = RepoInfo::Normal;
        }
        else {
            // impossible
            seaf_ext_log("bad repo status \"%s\"", status.c_str());
            continue;
        }
        // seaf_ext_log ("status for %s is \"%s\"", repo_name.c_str(),
        // status.c_str());
        infos->push_back(RepoInfo(repo_id, repo_name, worktree, st,
                                  support_file_lock, support_private_share));
    }

    reposInfoTimestamp = utils::currentMSecsSinceEpoch();
    return true;
}

GetFileStatusCommand::GetFileStatusCommand(const std::string& repo_id,
                                           const std::string& path_in_repo,
                                           bool isdir)
    : AppletCommand<RepoInfo::Status>("get-file-status"),
    repo_id_(repo_id),
    path_in_repo_(path_in_repo),
    isdir_(isdir)
{
}

std::string GetFileStatusCommand::serialize()
{
    char buf[512];
    snprintf (buf, sizeof(buf), "%s\t%s\t%s",
              repo_id_.c_str(), path_in_repo_.c_str(), isdir_ ? "true" : "false");
    return buf;
}

bool GetFileStatusCommand::parseResponse(const std::string& raw_resp,
                                         RepoInfo::Status *status)
{
    // seaf_ext_log ("raw_resp is %s\n", raw_resp.c_str());

    if (raw_resp == "syncing") {
        *status = RepoInfo::Syncing;
    } else if (raw_resp == "synced") {
        *status = RepoInfo::Normal;
    } else if (raw_resp == "error") {
        *status = RepoInfo::Error;
    } else if (raw_resp == "paused") {
        *status = RepoInfo::Paused;
    } else if (raw_resp == "readonly") {
        *status = RepoInfo::ReadOnly;
    } else if (raw_resp == "locked") {
        *status = RepoInfo::LockedByOthers;
    } else if (raw_resp == "locked_by_me") {
        *status = RepoInfo::LockedByMe;
    } else if (raw_resp == "ignored") {
        *status = RepoInfo::NoStatus;
    } else {
        *status = RepoInfo::NoStatus;

        // seaf_ext_log ("[GetFileStatusCommand] status for %s is %s, raw_resp is %s\n",
        //               path_in_repo_.c_str(),
        //               seafile::toString(*status).c_str(), raw_resp.c_str());
    }

    return true;
}

LockFileCommand::LockFileCommand(const std::string& path)
    : AppletCommand<void>("lock-file"),
    path_(path)
{
}

std::string LockFileCommand::serialize()
{
    return path_;
}

UnlockFileCommand::UnlockFileCommand(const std::string& path)
    : AppletCommand<void>("unlock-file"),
    path_(path)
{
}

std::string UnlockFileCommand::serialize()
{
    return path_;
}

PrivateShareCommand::PrivateShareCommand(const std::string& path, bool to_group)
    : AppletCommand<void>(to_group ? "private-share-to-group"
                                   : "private-share-to-user"),
      path_(path)
{
}

std::string PrivateShareCommand::serialize()
{
    return path_;
}

ShowHistoryCommand::ShowHistoryCommand(const std::string& path)
    : AppletCommand<void>("show-history"),
      path_(path)
{
}

std::string ShowHistoryCommand::serialize()
{
    return path_;
}

} // namespace seafile
