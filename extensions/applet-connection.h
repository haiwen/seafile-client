#ifndef SEAFILE_EXTENSION_APPLET_CONNECTION_H
#define SEAFILE_EXTENSION_APPLET_CONNECTION_H

#include <string>
#include "ext-utils.h"

namespace seafile {

/**
 * Connection to the seafile appplet, thourgh which the shell extension would
 * execute an `AppletCommand`.
 *
 * It connects to seafile appelt through a named pipe.
 */
class AppletConnection {
public:
    static AppletConnection *instance();

    bool prepare();
    bool connect();

    /**
     * Send the command in a separate thread, returns immediately
     */
    bool sendCommand(const std::string& data);

    /**
     * Send the command and blocking wait for response.
     */
    bool sendCommandAndWait(const std::string& data, std::string *resp);

private:
    AppletConnection();
    bool readResponse(std::string *out);
    bool writeRequest(const std::string& cmd);

    static AppletConnection *singleton_;

    bool connected_;
    HANDLE pipe_;

    /**
     * We have only one connection for each explorer process, so when sending
     * a command to seafile client we need to ensure exclusive access.
     */
    utils::Mutex mutex_;
    OVERLAPPED ol_;
};

} // namespace seafile

#endif // SEAFILE_EXTENSION_APPLET_CONNECTION_H
