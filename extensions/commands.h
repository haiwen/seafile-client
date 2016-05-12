#ifndef SEAFILE_EXTENSION_APPLET_COMMANDS_H
#define SEAFILE_EXTENSION_APPLET_COMMANDS_H

#include <string>
#include <vector>

#include "applet-connection.h"

namespace seafile {
class RepoInfo
{
public:
    enum Status {
        NoStatus = 0,
        Paused,
        Normal,
        Syncing,
        Error,
        LockedByMe,
        LockedByOthers,
        ReadOnly,
        N_Status,
    };

    std::string repo_id;
    std::string repo_name;
    std::string worktree;
    Status status;
    bool support_file_lock;
    bool support_private_share;

    RepoInfo() : status(NoStatus)
    {
    }

    RepoInfo(const std::string& repo_id,
             const std::string repo_name,
             const std::string& worktree,
             Status status,
             bool support_file_lock,
             bool support_private_share)
        : repo_id(repo_id),
          repo_name(repo_name),
          worktree(worktree),
          status(status),
          support_file_lock(support_file_lock),
          support_private_share(support_private_share)
    {
    }

    bool isValid()
    {
        return !repo_id.empty();
    }
};

std::string toString(RepoInfo::Status st);

typedef std::vector<RepoInfo> RepoInfoList;

/**
 * Abstract base class for all commands sent to seafile applet.
 */
template<class T>
class AppletCommand {
public:
    AppletCommand(std::string name) : name_(name) {}

    /**
     * send the command to seafile client, don't need the response
     */
    void send()
    {
        AppletConnection::instance()->sendCommand(formatRequest());
    }

    std::string formatRequest()
    {
        return name_ + "\t" + serialize();
    }

    /**
     * send the command to seafile client, and wait for the response
     */
    bool sendAndWait(T *resp)
    {
        std::string raw_resp;
        if (!AppletConnection::instance()->sendCommandAndWait(formatRequest(), &raw_resp)) {
            return false;
        }

        return parseResponse(raw_resp, resp);
    }

protected:
    /**
     * Prepare this command for sending through the pipe
     */
    virtual std::string serialize() = 0;

    /**
     * Parse response from seafile applet. Commands that don't need the
     * respnse can inherit the implementation of the base class, which does
     * nothing.
     */
    virtual bool parseResponse(const std::string& raw_resp, T *resp)
    {
        return true;
    }

private:
    std::string name_;
};


class GetShareLinkCommand : public AppletCommand<void> {
public:
    GetShareLinkCommand(const std::string path);

protected:
    std::string serialize();

private:
    std::string path_;
};

class GetInternalLinkCommand : public AppletCommand<void> {
public:
    GetInternalLinkCommand(const std::string path);

protected:
    std::string serialize();

private:
    std::string path_;
};


class ListReposCommand : public AppletCommand<RepoInfoList> {
public:
    ListReposCommand();

protected:
    std::string serialize();

    bool parseResponse(const std::string& raw_resp, RepoInfoList *worktrees);
};

class GetFileStatusCommand : public AppletCommand<RepoInfo::Status> {
public:
    GetFileStatusCommand(const std::string& repo_id, const std::string& path_in_repo, bool isdir);

protected:
    std::string serialize();

    bool parseResponse(const std::string& raw_resp, RepoInfo::Status *status);

private:
    std::string repo_id_;
    std::string path_in_repo_;
    bool isdir_;
};

class LockFileCommand : public AppletCommand<void> {
public:
    LockFileCommand(const std::string& path);

protected:
    std::string serialize();

private:
    std::string path_;
};

class UnlockFileCommand : public AppletCommand<void> {
public:
    UnlockFileCommand(const std::string& path);

protected:
    std::string serialize();

private:
    std::string path_;
};

class PrivateShareCommand : public AppletCommand<void> {
public:
    PrivateShareCommand(const std::string& path, bool to_group);

protected:
    std::string serialize();

private:
    std::string path_;
    bool to_group;
};

class ShowHistoryCommand : public AppletCommand<void> {
public:
    ShowHistoryCommand(const std::string& path);

protected:
    std::string serialize();

private:
    std::string path_;
};

}

#endif // SEAFILE_EXTENSION_APPLET_COMMANDS_H
