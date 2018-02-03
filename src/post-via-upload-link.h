#ifndef SEAFILE_CLIENT_POST_VIA_UPLOAD_LINK_H_
#define SEAFILE_CLIENT_POST_VIA_UPLOAD_LINK_H_

#include <QObject>
#include <QString>
#include <QNetworkReply>
#include "filebrowser/tasks.h"

class SeafileTrayIcon;

class PostToUploadLink : public FileServerTask {
    friend class SeafileTrayIcon;
public:
    PostToUploadLink(const QString url, const QString local_file);
    QNetworkReply *getReply() const { return reply_; }

// signals:
//     void progressUpdate(qint64 transferred, qint64 total);
//     void finished(bool success);

protected:
    void prepare();
    void sendRequest();
    void onHttpRequestFinished();

private:
    QFile *file_;
};

#endif // SEAFILE_CLIENT_POST_VIA_UPLOAD_LINK_H_
