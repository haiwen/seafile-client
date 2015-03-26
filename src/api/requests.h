#ifndef SEAFILE_CLIENT_API_REQUESTS_H
#define SEAFILE_CLIENT_API_REQUESTS_H

#include <vector>
#include <QMap>

#include "api-request.h"
#include "server-repo.h"
#include "account.h"

class QNetworkReply;
class QImage;

class ServerRepo;
class Account;
class StarredFile;
class SeafEvent;
class CommitDetails;

class PingServerRequest : public SeafileApiRequest {
    Q_OBJECT
public:
    PingServerRequest(const QUrl& serverAddr);

protected slots:
    void requestSuccess(QNetworkReply& reply);

signals:
    void success();

private:
    Q_DISABLE_COPY(PingServerRequest)
};

class LoginRequest : public SeafileApiRequest {
    Q_OBJECT

public:
    LoginRequest(const QUrl& serverAddr,
                 const QString& username,
                 const QString& password,
                 const QString& computer_name);

protected slots:
    void requestSuccess(QNetworkReply& reply);

signals:
    void success(const QString& token);

private:
    Q_DISABLE_COPY(LoginRequest)
};


class ListReposRequest : public SeafileApiRequest {
    Q_OBJECT

public:
    explicit ListReposRequest(const Account& account);

protected slots:
    void requestSuccess(QNetworkReply& reply);

signals:
    void success(const std::vector<ServerRepo>& repos);

private:
    Q_DISABLE_COPY(ListReposRequest)
};


class RepoDownloadInfo {
public:
    int repo_version;
    QString relay_id;
    QString relay_addr;
    QString relay_port;
    QString email;
    QString token;
    QString repo_id;
    QString repo_name;
    bool encrypted;
    bool readonly;
    int enc_version;
    QString magic;
    QString random_key;
    QString more_info;

    static RepoDownloadInfo fromDict(QMap<QString, QVariant>& dict,
                                     const QUrl& url,
                                     bool read_only);
};

class DownloadRepoRequest : public SeafileApiRequest {
    Q_OBJECT

public:
    explicit DownloadRepoRequest(const Account& account,
                                 const QString& repo_id,
                                 bool read_only);

protected slots:
    void requestSuccess(QNetworkReply& reply);

signals:
    void success(const RepoDownloadInfo& info);

private:
    Q_DISABLE_COPY(DownloadRepoRequest)

    bool read_only_;
};

class CreateRepoRequest : public SeafileApiRequest {
    Q_OBJECT

public:
    explicit CreateRepoRequest(const Account& account, QString &name, QString &desc, QString &passwd);

protected slots:
    void requestSuccess(QNetworkReply& reply);

signals:
    void success(const RepoDownloadInfo& info);

private:
    Q_DISABLE_COPY(CreateRepoRequest)

};

class GetUnseenSeahubNotificationsRequest : public SeafileApiRequest {
    Q_OBJECT

public:
    explicit GetUnseenSeahubNotificationsRequest(const Account& account);

protected slots:
    void requestSuccess(QNetworkReply& reply);

signals:
    void success(int count);

private:
    Q_DISABLE_COPY(GetUnseenSeahubNotificationsRequest)

};

class GetDefaultRepoRequest : public SeafileApiRequest {
    Q_OBJECT
public:
    GetDefaultRepoRequest(const Account& account);

signals:
    void success(bool exists, const QString& repo_id);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(GetDefaultRepoRequest)
};

class CreateDefaultRepoRequest : public SeafileApiRequest {
    Q_OBJECT
public:
    CreateDefaultRepoRequest(const Account& account);

signals:
    void success(const QString& repo_id);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(CreateDefaultRepoRequest)
};

class GetLatestVersionRequest : public SeafileApiRequest {
    Q_OBJECT
public:
    GetLatestVersionRequest(const QString& client_id, const QString& client_version);

signals:
    void success(const QString& latest_version);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(GetLatestVersionRequest)
};

class GetStarredFilesRequest : public SeafileApiRequest {
    Q_OBJECT
public:
    GetStarredFilesRequest(const Account& account);

signals:
    void success(const std::vector<StarredFile>& starred_files);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(GetStarredFilesRequest)
};

class GetEventsRequest : public SeafileApiRequest {
    Q_OBJECT
public:
    GetEventsRequest(const Account& account, int start=0);

signals:
    void success(const std::vector<SeafEvent>& events, int more_offset);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(GetEventsRequest)
};

class GetCommitDetailsRequest : public SeafileApiRequest {
    Q_OBJECT
public:
    GetCommitDetailsRequest(const Account& account,
                            const QString& repo_id,
                            const QString& commit_id);

signals:
    void success(const CommitDetails& result);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(GetCommitDetailsRequest)
};

class FetchImageRequest : public SeafileApiRequest {
    Q_OBJECT
public:
    FetchImageRequest(const QString& img_url);

signals:
    void success(const QImage& avatar);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(FetchImageRequest)
};

class GetAvatarRequest : public SeafileApiRequest {
    Q_OBJECT
public:
    GetAvatarRequest(const Account& account,
                     const QString& email,
                     int size);

    ~GetAvatarRequest();

    const QString& email() const { return email_; }
    const Account& account() const { return account_; }

signals:
    void success(const QImage& avatar);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(GetAvatarRequest)

    FetchImageRequest *fetch_img_req_;

    QString email_;

    Account account_;
};

class SetRepoPasswordRequest : public SeafileApiRequest {
    Q_OBJECT
public:
    SetRepoPasswordRequest(const Account& account,
                           const QString& repo_id,
                           const QString& password);

signals:
    void success();

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(SetRepoPasswordRequest)
};

#endif // SEAFILE_CLIENT_API_REQUESTS_H
