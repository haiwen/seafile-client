#include "ext-common.h"
#include "log.h"
#include "applet-connection.h"
#include "ext-utils.h"

#include "commands.h"

namespace seafile {


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
    : AppletCommand<WorktreeList>("list-repos")
{
}

std::string ListReposCommand::serialize()
{
    return "";
}

bool ListReposCommand::parseResponse(const std::string& raw_resp,
                                     WorktreeList *worktrees)
{
    std::vector<std::string> lines = utils::split(raw_resp, '\n');
    if (lines.empty()) {
        return true;
    }
    for (size_t i = 0; i < lines.size(); i++) {
        std::string line = lines[i];
        std::vector<std::string> parts = utils::split(line, '\t');
        if (parts.size() != 2) {
            continue;
        }
        worktrees->push_back(utils::normalizedPath(parts[1]));
    }
    return true;
}

} // namespace seafile
