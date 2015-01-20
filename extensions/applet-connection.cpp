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
      pipe_(INVALID_HANDLE_VALUE)
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
        seaf_ext_log("Failed to create named pipe: %s", utils::formatErrorMessage().c_str());
        connected_ = false;
        return false;
    }

    connected_ = true;
    seaf_ext_log("connected to seafile client");
    return true;
}

bool
AppletConnection::prepare()
{
    memset(&ol_, 0, sizeof(ol_));
    HANDLE h_ev = CreateEvent
        (NULL,                  /* security attribute */
         FALSE,                 /* manual reset */
         FALSE,                 /* initial state  */
         NULL);                 /* event name */

    if (!h_ev) {
        return false;
    }
    ol_.hEvent = h_ev;
    return true;
}


bool AppletConnection::sendCommand(const std::string& cmd)
{
    seaf_ext_log("Thread (%lu) send command: %s", GetCurrentThreadId(), cmd.c_str());

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

bool AppletConnection::writeRequest(const std::string& cmd)
{
    uint32_t len = cmd.size();
    seaf_ext_log("Thread (%lu) communicate: %s", GetCurrentThreadId(), cmd.c_str());
    if (!utils::pipeWriteN(pipe_, &len, sizeof(len), &ol_, &connected_)) {
        seaf_ext_log("failed to send command: %s", utils::formatErrorMessage().c_str());
        return false;
    }

    if (!utils::pipeWriteN(pipe_, cmd.c_str(), len, &ol_, &connected_)) {
        seaf_ext_log("failed to send command: %s", utils::formatErrorMessage().c_str());
        return false;
    }
    return true;
}

bool AppletConnection::readResponse(std::string *out)
{
    uint32_t len;
    if (!utils::pipeReadN(pipe_, &len, sizeof(len), &ol_, &connected_) || len == 0) {
        return false;
    }

    std::shared_ptr<char> buf(new char[len + 1]);
    buf.get()[len] = 0;
    if (!utils::pipeReadN(pipe_, buf.get(), len, &ol_, &connected_)) {
        return false;
    }

    if (out != NULL) {
        *out = buf.get();
    }

    return true;
}

bool AppletConnection::sendWithReconnect(const std::string& cmd)
{
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
