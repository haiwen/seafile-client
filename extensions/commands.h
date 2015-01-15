#ifndef SEAFILE_EXTENSION_APPLET_COMMANDS_H
#define SEAFILE_EXTENSION_APPLET_COMMANDS_H

#include <string>
#include <vector>

#include "applet-connection.h"

namespace seafile {

typedef std::vector<std::string> WorktreeList;

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


class ListReposCommand : public AppletCommand<WorktreeList> {
public:
    ListReposCommand();

protected:
    std::string serialize();

    bool parseResponse(const std::string& raw_resp, WorktreeList *worktrees);
};


}

#endif // SEAFILE_EXTENSION_APPLET_COMMANDS_H
