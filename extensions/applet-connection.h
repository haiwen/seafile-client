#ifndef SEAFILE_EXTENSION_APPLET_CONNECTION_H
#define SEAFILE_EXTENSION_APPLET_CONNECTION_H

#include <string>

namespace seafile {

class AppletCommand;

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

    bool sendCommand(const std::string& data);
    bool communicate(const std::string& data);

private:
    AppletConnection();

    static AppletConnection *singleton_;

    class Lock {
    };

    bool connected_;
    HANDLE pipe_;

    /**
     * We have only one connection for each explorer process, so when sending
     * a command to seafile client we need to ensure exclusive access.
     */
    Lock lock_;
    OVERLAPPED ol_;
};

}

#endif // SEAFILE_EXTENSION_APPLET_CONNECTION_H
