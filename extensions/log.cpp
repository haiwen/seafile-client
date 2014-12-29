
#include <ctime>

#include "ext-common.h"

#include "log.h"

Logger* Logger::singleton_;

Logger* Logger::instance()
{
    if (singleton_ == NULL) {
        singleton_ = new Logger;
    }

    return singleton_;
}

Logger::Logger()
{
    ofs_.open("C:/Users/lin-vm/seaf_ext.log");

    UINT mtype = MB_OK | MB_ICONWARNING;

    MessageBox(NULL, "hello world", "Seafile", mtype);
}

Logger::~Logger()
{
}

void Logger::log(std::string msg, Level lvl)
{
    Logger *logger = instance();

    logger->write(msg, lvl);
}

void Logger::write(std::string msg, Level lvl)
{
    time_t t;
    struct tm *tm;
    char buf[256];
    std::string lvlstr;

    switch(lvl) {
    case DEBUG:
        lvlstr = "DEBUG";
        break;
    case INFO:
        lvlstr = "INFO";
        break;
    case WARNING:
        lvlstr = "WARNING";
        break;
    case FATAL:
        lvlstr = "FATAL";
        break;
    }

    lvlstr = "[" + lvlstr + "]";

    t = time(NULL);
    tm = localtime(&t);
    strftime (buf, 256, "[%y/%m/%d %H:%M:%S] ", tm);

    ofs_ << (char *)buf;
    ofs_ << lvlstr;
    ofs_ << msg << std::endl;
    ofs_.flush();
}
