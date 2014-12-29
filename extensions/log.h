#ifndef SEAFILE_CLIENT_EXTENSION_LOG_H
#define SEAFILE_CLIENT_EXTENSION_LOG_H

#include <fstream>

class Logger {
public:
    enum Level {
        DEBUG,
        INFO,
        WARNING,
        FATAL,
    };

    static Logger* instance();
    ~Logger();

    static void debug(std::string msg) { log(msg, DEBUG); }
    static void info(std::string msg) { log(msg, INFO); }
    static void warning(std::string msg) { log(msg, WARNING); }
    static void fatal(std::string msg) { log(msg, FATAL); }

private:
    Logger();

    void write(std::string msg, Level lvl);

    static void log(std::string msg, Level lvl);

    static Logger *singleton_;

    std::ofstream ofs_;
};


#endif  // SEAFILE_CLIENT_EXTENSION_LOG_H
