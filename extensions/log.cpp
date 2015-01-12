#include "ext-common.h"
#include <time.h>
#include <stdarg.h>

#include "ext-utils.h"
#include "log.h"

namespace {

static FILE *log_fp;

std::string getLogPath()
{
    std::string home = seafile::utils::getHomeDir();
    if (home.empty())
        return "";

    return home + "/seaf_ext.log";
}


}

void
seaf_ext_log_start ()
{
    if (log_fp)
        return;

    std::string log_path = getLogPath();
    if (!log_path.empty())
        log_fp = fopen (log_path.c_str(), "a");

    if (log_fp) {
        seaf_ext_log ("\n----------------------------------\n"
                      "log file initialized: %s"
                      "\n----------------------------------\n"
                      , log_path.c_str());
    } else {
        fprintf (stderr, "[LOG] Can't init log file %s\n", log_path.c_str());
    }
}

void
seaf_ext_log_stop ()
{
    if (log_fp) {
        fclose (log_fp);
        log_fp = NULL;
    }
}

void
seaf_ext_log_aux (const char *format, ...)
{
    if (!log_fp)
        seaf_ext_log_start();

    if (log_fp) {
        va_list params;
        char buffer[1024];
        size_t length = 0;

        va_start(params, format);
        length = vsnprintf(buffer, sizeof(buffer), format, params);
        va_end(params);

        /* Write the timestamp. */
        time_t t;
        struct tm *tm;
        char buf[256];

        t = time(NULL);
        tm = localtime(&t);
        strftime (buf, 256, "[%y/%m/%d %H:%M:%S] ", tm);

        fputs (buf, log_fp);
        if (fwrite(buffer, sizeof(char), length, log_fp) < length)
            return;

        fputc('\n', log_fp);
        fflush(log_fp);
    }
}
