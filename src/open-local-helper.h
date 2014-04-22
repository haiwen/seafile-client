#ifndef SEAFILE_CLIENT_OPEN_LOCAL_FILE_HELPER_H
#define SEAFILE_CLIENT_OPEN_LOCAL_FILE_HELPER_H

extern "C" {

struct _CcnetClient;

}

/**
 * Helper class to handle open local file request
 */
class OpenLocalHelper
{
public:
    static OpenLocalHelper* instance();

    void openLocalFile(const char* url);

    void handleOpenLocalFromCommandLine(const char *url);

    void checkPendingOpenLocalRequest();

private:
    static OpenLocalHelper* singleton_;

    OpenLocalHelper();

    void messageBox(const QString& msg);

    bool connectToCcnetDaemon();

    void sendOpenLocalFileMessage(const char *url);

    _CcnetClient *sync_client_;

    char *url_;
};


#endif // SEAFILE_CLIENT_OPEN_LOCAL_FILE_HELPER_H
