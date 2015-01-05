#include "crash-handler.h"
#include <QDir>
#include <QString>
#include <cstdio>

#if defined(Q_WS_X11)
#include "client/linux/handler/exception_handler.h"
#elif defined(Q_WS_WIN)
#include "client/windows/handler/exception_handler.h"
#elif defined(Q_WS_MAC)
#include "client/mac/handler/exception_handler.h"
#endif

namespace Breakpad {
//
//  Implementation for Crash Handler
//
class CrashHandlerPrivate
{
public:
    CrashHandlerPrivate() {}
    ~CrashHandlerPrivate() { delete handler; }

    void InitCrashHandler(const QString& dumpPath);

#if defined(Q_WS_WIN)
    static bool DumpCallback(const wchar_t* _dump_dir, const wchar_t* _minidump_id, void* context, EXCEPTION_POINTERS* exinfo, MDRawAssertionInfo* assertion, bool success);
#elif defined(Q_WS_X11)
    static bool DumpCallback(const google_breakpad::MinidumpDescriptor &md, void *context, bool success);
#elif defined(Q_WS_MAC)
    static bool DumpCallback(const char* _dump_dir,const char* _minidump_id,void *context, bool success);
#endif

    static google_breakpad::ExceptionHandler* handler;
};

google_breakpad::ExceptionHandler* CrashHandlerPrivate::handler = NULL;

#if defined(Q_WS_WIN)
bool CrashHandlerPrivate::DumpCallback(const wchar_t* _dump_dir, const wchar_t* _minidump_id, void* context, EXCEPTION_POINTERS* exinfo, MDRawAssertionInfo* assertion, bool success);
#elif defined(Q_WS_X11)
bool CrashHandlerPrivate::DumpCallback(const google_breakpad::MinidumpDescriptor &md, void *context, bool success)
#elif defined(Q_WS_MAC)
bool CrashHandlerPrivate::DumpCallback(const char* _dump_dir,const char* _minidump_id,void *context, bool success)
#endif
{
    Q_UNUSED(context);
#if defined(Q_WS_WIN)
    Q_UNUSED(_dump_dir);
    Q_UNUSED(_minidump_id);
    Q_UNUSED(assertion);
    Q_UNUSED(exinfo);
#endif
    fprintf(stderr, "[breakpad] crash detected\n");
    /*
     * unsafe context
     * don't allocate memory by heap
     *
     */

    // CrashHandlerPrivate* self = static_cast<CrashHandlerPrivate*>(context);

    if (!success) {
      fprintf(stderr, "Failed to generate minidump.\n");
      return false;
    }

    fprintf(stderr, "[breakpad] minidump generated\n");

    return success;
}

    void CrashHandlerPrivate::InitCrashHandler(const QString& dumpPath)
    {
        if (handler != NULL)
            return;

#if defined(Q_WS_WIN)
        std::wstring pathAsStr = (const wchar_t*)dumpPath.utf16();
        handler = new google_breakpad::ExceptionHandler(
            pathAsStr,
            /*FilterCallback*/ 0,
            DumpCallback,
            /*context*/ this,
            true
            );
#elif defined(Q_WS_X11)
        std::string pathAsStr = dumpPath.toStdString();
        google_breakpad::MinidumpDescriptor md(pathAsStr);
        handler = new google_breakpad::ExceptionHandler(
            md,
            /*FilterCallback*/ 0,
            DumpCallback,
            /*context*/ this,
            true,
            -1
            );
#elif defined(Q_WS_MAC)
        std::string pathAsStr = dumpPath.toStdString();
        handler = new google_breakpad::ExceptionHandler(
            pathAsStr,
            /*FilterCallback*/ 0,
            DumpCallback,
            /*context*/ this,
            true,
            NULL
            );
#endif
        fprintf(stderr, "[breakpad] initialized\n");
    }

    CrashHandler* CrashHandler::instance()
    {
        static CrashHandler globalHandler;
        return &globalHandler;
    }

    CrashHandler::CrashHandler()
    {
        d = new CrashHandlerPrivate();
    }

    CrashHandler::~CrashHandler()
    {
        delete d;
    }

    void CrashHandler::Init(const QString& reportPath)
    {
        if (!QDir(reportPath).mkpath("."))
            fprintf(stderr, "[breakpad] failed to create crash directory\n");
        d->InitCrashHandler(reportPath);
    }
}

