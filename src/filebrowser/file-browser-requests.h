#ifndef SEAFILE_CLIENT_FILE_BROWSER_REQUESTS_H
#define SEAFILE_CLIENT_FILE_BROWSER_REQUESTS_H

#include <QList>

#include "api/api-request.h"
#include "seaf-dirent.h"

class SeafDirent;
class Account;

class GetDirentsRequest : public SeafileApiRequest {
    Q_OBJECT
public:
    GetDirentsRequest(const Account& account,
                      const QString& repo_id,
                      const QString& path,
                      const QString& cached_dir_id=QString());

    QString repoId() const { return repo_id_; }
    QString path() const { return path_; }

signals:
    void success(const QString& dir_id,
                 const QList<SeafDirent>& dirents);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(GetDirentsRequest);

    QString repo_id_;
    QString path_;
    QString cached_dir_id_;
};

#endif  // SEAFILE_CLIENT_FILE_BROWSER_REQUESTS_H
