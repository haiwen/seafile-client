#define __STDC_LIMIT_MACROS
#include "ext-common.h"

#include <memory>

#include "ext-utils.h"
#include "log.h"

#include "applet-connection.h"

namespace {

const char *kSeafExtPipeName = "\\\\.\\pipe\\seafile_ext_pipe";

struct ThreadData {
    seafile::AppletConnection *conn;
    std::string cmd;
};

DWORD WINAPI sendDataWrapper (void *vdata)
{
    ThreadData *tdata = (ThreadData *)vdata;
    bool ret = tdata->conn->sendCommandAndWait(tdata->cmd, NULL);
    delete tdata;
    ExitThread(ret);
    return 0;
}

} // namespace

namespace seafile {

AppletConnection::AppletConnection()
    : connected_(false),
      pipe_(INVALID_HANDLE_VALUE),
      last_conn_failure_(0)
{
}

AppletConnection *AppletConnection::singleton_;

AppletConnection *AppletConnection::instance()
{
    if (!singleton_) {
        static AppletConnection v;
        singleton_ = &v;
    }
    return singleton_;
}


bool
AppletConnection::connect ()
{
    if (pipe_ != INVALID_HANDLE_VALUE) {
        CloseHandle (pipe_);
    }
    pipe_ = CreateFile(
        kSeafExtPipeName,       // pipe name
        GENERIC_READ |          // read and write access
        GENERIC_WRITE,
        0,                      // no sharing
        NULL,                   // default security attributes
        OPEN_EXISTING,          // opens existing pipe
        FILE_FLAG_OVERLAPPED,   // default attributes
        NULL);                  // no template file

    if (pipe_ == INVALID_HANDLE_VALUE) {
        if (GetLastError() != ERROR_FILE_NOT_FOUND) {
            seaf_ext_log("Failed to create named pipe: %s", utils::formatErrorMessage().c_str());
        }
        connected_ = false;
        last_conn_failure_ = utils::currentMSecsSinceEpoch();
        return false;
    }

    DWORD mode = PIPE_READMODE_MESSAGE;
    if (!SetNamedPipeHandleState(pipe_, &mode, NULL, NULL)) {
        seaf_ext_log("Failed to set named pipe mode: %s", utils::formatErrorMessage().c_str());
        onPipeError();
        last_conn_failure_ = utils::currentMSecsSinceEpoch();
        return false;
    }

    connected_ = true;
    return true;
}

bool
AppletConnection::prepare()
{
    // HANDLE h_ev = CreateEvent
    //     (NULL,                  /* security attribute */
    //      FALSE,                 /* manual reset */
    //      FALSE,                 /* initial state  */
    //      NULL);                 /* event name */

    // if (!h_ev) {
    //     return false;
    // }
    return true;
}


bool AppletConnection::sendCommand(const std::string& cmd)
{
    ThreadData *tdata = new ThreadData;
    tdata->conn = this;
    tdata->cmd = cmd;
    utils::doInThread((LPTHREAD_START_ROUTINE)sendDataWrapper, (void *)tdata);
    return true;
}

bool AppletConnection::sendCommandAndWait(const std::string& cmd, std::string *resp)
{
    utils::MutexLocker lock(&mutex_);
    if (!sendWithReconnect(cmd)) {
        return false;
    }

    std::string r;
    if (!readResponse(&r)) {
        return false;
    }

    if (resp != NULL) {
        *resp = r;
    }

    return true;
}

void AppletConnection::onPipeError()
{
    CloseHandle(pipe_);
    pipe_ = INVALID_HANDLE_VALUE;
    connected_ = false;
}

bool AppletConnection::writeRequest(const std::string& cmd)
{
    uint32_t len = cmd.size();
    if (!utils::pipeWriteN(pipe_, &len, sizeof(len))) {
        onPipeError();
        seaf_ext_log("failed to send command: %s", utils::formatErrorMessage().c_str());
        return false;
    }

    if (!utils::pipeWriteN(pipe_, cmd.c_str(), len)) {
        onPipeError();
        seaf_ext_log("failed to send command: %s", utils::formatErrorMessage().c_str());
        return false;
    }
    return true;
}

bool AppletConnection::readResponse(std::string *out)
{
    uint32_t len = 0;
    if (!utils::pipeReadN(pipe_, &len, sizeof(len))) {
        onPipeError();
        return false;
    }

    // avoid integer overflow
    if (len == UINT32_MAX) {
        return false;
    }

    if (len == 0) {
        return true;
    }

    std::unique_ptr<char[]> buf(new char[len + 1]);
    buf.get()[len] = 0;
    if (!utils::pipeReadN(pipe_, buf.get(), len)) {
        onPipeError();
        return false;
    }

    if (out != NULL) {
        *out = buf.get();
    }

    return true;
}

bool AppletConnection::sendWithReconnect(const std::string& cmd)
{
    uint64_t now = utils::currentMSecsSinceEpoch();
    if (!connected_ && now - last_conn_failure_ < 2000) {
        return false;
    }
    if (!connected_) {
        if (connect() && writeRequest(cmd)) {
            return true;
        }
    } else {
        if (writeRequest(cmd)) {
            return true;
        } else if (!connected_ && connect()) {
            // Retry one more time when connection is broken. This normally
            // happens when seafile client was restarted.
            seaf_ext_log ("reconnected to seafile cient");
            if (writeRequest(cmd)) {
                return true;
            }
        }
    }

    return false;
}

} // namespace seafile
