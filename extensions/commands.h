#ifndef SEAFILE_EXTENSION_APPLET_COMMANDS_H
#define SEAFILE_EXTENSION_APPLET_COMMANDS_H

#include <string>

namespace seafile {

/**
 * Abstract base class for all commands sent to seafile applet.
 */
class AppletCommand {
public:
    AppletCommand(std::string name);

    /**
     * send the command to seafile client
     */
    void send();

    /**
     * prepare this command for sending through the pipe
     */
    virtual std::string serialize() = 0;

private:
    std::string name_;
};

class GetShareLinkCommand : public AppletCommand {
public:
    GetShareLinkCommand(const std::string path);

    std::string serialize();

private:
    std::string path_;
};

}

#endif // SEAFILE_EXTENSION_APPLET_COMMANDS_H
