#ifndef SEAFILE_CLIENT_API_REQUESTS_H
#define SEAFILE_CLIENT_API_REQUESTS_H

#include <QMap>
#include <QScopedPointer>
#include <vector>

#include "account.h"
#include "api-request.h"
#include "contact-share-info.h"
#include "server-repo.h"
#include "server-repo.h"

class QNetworkReply;
class QImage;
class QStringList;

class ServerRepo;
class Account;
class StarredFile;
class SeafEvent;
class CommitDetails;

class PingServerRequest : public SeafileApiRequest
{
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

class LoginRequest : public SeafileApiRequest
{
    Q_OBJECT

public:
    LoginRequest(const QUrl& serverAddr,
                 const QString& username,
                 const QString& password,
                 const QString& computer_name);

protected slots:
    void requestSuccess(QNetworkReply& reply);

signals:
    void success(const QString& token, const QString& s2fa_token);

private:
    Q_DISABLE_COPY(LoginRequest)
};


class ListReposRequest : public SeafileApiRequest
{
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


class RepoDownloadInfo
{
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

class DownloadRepoRequest : public SeafileApiRequest
{
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

class GetRepoRequest : public SeafileApiRequest
{
    Q_OBJECT

public:
    explicit GetRepoRequest(const Account& account, const QString& repoid);
    const QString& repoid()
    {
        return repoid_;
    }

protected slots:
    void requestSuccess(QNetworkReply& reply);

signals:
    void success(const ServerRepo& repo);

private:
    Q_DISABLE_COPY(GetRepoRequest)
    const QString repoid_;
};

class CreateRepoRequest : public SeafileApiRequest
{
    Q_OBJECT

public:
    CreateRepoRequest(const Account& account,
                      const QString& name,
                      const QString& desc,
                      const QString& passwd);
    CreateRepoRequest(const Account& account,
                      const QString& name,
                      const QString& desc,
                      int enc_version,
                      const QString& repo_id,
                      const QString& magic,
                      const QString& random_key);

protected slots:
    void requestSuccess(QNetworkReply& reply);

signals:
    void success(const RepoDownloadInfo& info);

private:
    Q_DISABLE_COPY(CreateRepoRequest)
};

class CreateSubrepoRequest : public SeafileApiRequest
{
    Q_OBJECT

public:
    explicit CreateSubrepoRequest(const Account& account,
                                  const QString& name,
                                  const QString& repoid,
                                  const QString& path,
                                  const QString& passwd);

protected slots:
    void requestSuccess(QNetworkReply& reply);

signals:
    void success(const QString& sub_repoid);

private:
    Q_DISABLE_COPY(CreateSubrepoRequest)
};

class GetUnseenSeahubNotificationsRequest : public SeafileApiRequest
{
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

class GetDefaultRepoRequest : public SeafileApiRequest
{
    Q_OBJECT
public:
    GetDefaultRepoRequest(const Account& account);

signals:
    void success(bool exists, const QString& repo_id);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(GetDefaultRepoRequest);
};

class CreateDefaultRepoRequest : public SeafileApiRequest
{
    Q_OBJECT
public:
    CreateDefaultRepoRequest(const Account& account);

signals:
    void success(const QString& repo_id);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(CreateDefaultRepoRequest);
};

class GetStarredFilesRequest : public SeafileApiRequest
{
    Q_OBJECT
public:
    GetStarredFilesRequest(const Account& account);

signals:
    void success(const std::vector<StarredFile>& starred_files);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(GetStarredFilesRequest);
};

class GetEventsRequest : public SeafileApiRequest
{
    Q_OBJECT
public:
    GetEventsRequest(const Account& account, int start = 0);

signals:
    void success(const std::vector<SeafEvent>& events, int more_offset);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(GetEventsRequest);
};

class GetCommitDetailsRequest : public SeafileApiRequest
{
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
    Q_DISABLE_COPY(GetCommitDetailsRequest);
};

class FetchImageRequest : public SeafileApiRequest
{
    Q_OBJECT
public:
    FetchImageRequest(const QString& img_url);

signals:
    void success(const QImage& avatar);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(FetchImageRequest);
};

class GetAvatarRequest : public SeafileApiRequest
{
    Q_OBJECT
public:
    GetAvatarRequest(const Account& account,
                     const QString& email,
                     qint64 mtime,
                     int size);

    ~GetAvatarRequest();

    const QString& email() const
    {
        return email_;
    }
    const Account& account() const
    {
        return account_;
    }
    qint64 mtime() const
    {
        return mtime_;
    }

signals:
    void success(const QImage& avatar);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(GetAvatarRequest);

    FetchImageRequest* fetch_img_req_;

    QString email_;

    Account account_;

    qint64 mtime_;
};

class SetRepoPasswordRequest : public SeafileApiRequest
{
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
    Q_DISABLE_COPY(SetRepoPasswordRequest);
};

class ServerInfoRequest : public SeafileApiRequest
{
    Q_OBJECT
public:
    ServerInfoRequest(const Account& account);

signals:
    void success(const Account& account, const ServerInfo& info);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(ServerInfoRequest);
    const Account& account_;
};

class LogoutDeviceRequest : public SeafileApiRequest
{
    Q_OBJECT
public:
    LogoutDeviceRequest(const Account& account);

    const Account& account()
    {
        return account_;
    }

signals:
    void success();

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(LogoutDeviceRequest);

    Account account_;
};

class SingleBatchRepoTokensRequest : public SeafileApiRequest
{
    Q_OBJECT
public:
    SingleBatchRepoTokensRequest(const Account& account, const QStringList& repo_ids);

    const QMap<QString, QString>& repoTokens() const
    {
        return repo_tokens_;
    }

    const QStringList repoIds() const {
        return repo_ids_;
    }

signals:
    void success();

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(SingleBatchRepoTokensRequest);

    QStringList repo_ids_;
    QMap<QString, QString> repo_tokens_;
};

// Request repo sync tokens from the server, and break the request into batches
// if there are too many, to avoid request URI too large.
class GetRepoTokensRequest : public SeafileApiRequest
{
    Q_OBJECT
public:
    GetRepoTokensRequest(const Account& account, const QStringList& repo_ids, int batch_size=50);

    virtual void send() Q_DECL_OVERRIDE;

    const QMap<QString, QString>& repoTokens() const
    {
        return repo_tokens_;
    }

signals:
    void success();

protected slots:
    void requestSuccess(QNetworkReply& reply);

private slots:
    void batchSuccess();

private:
    Q_DISABLE_COPY(GetRepoTokensRequest);

    void doNextBatch();

    Account account_;
    QStringList repo_ids_;
    QMap<QString, QString> repo_tokens_;

    // The start position of the next batch
    int batch_offset_;
    // How many tokens to ask in a single request
    int batch_size_;

    QScopedPointer<SingleBatchRepoTokensRequest, QScopedPointerDeleteLater> batch_req_;
};

class GetLoginTokenRequest : public SeafileApiRequest
{
    Q_OBJECT
public:
    GetLoginTokenRequest(const Account& account, const QString& next_url);

    const Account& account()
    {
        return account_;
    }
    const QString& nextUrl()
    {
        return next_url_;
    }

signals:
    void success(const QString& token);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(GetLoginTokenRequest);

    Account account_;
    QString next_url_;
};

struct FileSearchResult {
    QString repo_id;
    QString repo_name;
    QString name;
    QString oid;
    qint64 last_modified;
    QString fullpath;
    qint64 size;
};

Q_DECLARE_METATYPE(FileSearchResult)

class FileSearchRequest : public SeafileApiRequest
{
    Q_OBJECT
public:
    FileSearchRequest(const Account& account,
                      const QString& keyword,
                      int page = 0,
                      int per_page = 10,
                      const QString& repo_id = QString());
    const QString& keyword() const
    {
        return keyword_;
    }

signals:
    void success(const std::vector<FileSearchResult>& result,
                 bool is_loading_more,
                 bool has_more);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(FileSearchRequest);

    const QString keyword_;
    const int page_;
};

class FetchCustomLogoRequest : public SeafileApiRequest
{
    Q_OBJECT
public:
    FetchCustomLogoRequest(const QUrl& url);

signals:
    void success(const QUrl& url);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(FetchCustomLogoRequest);
};

class FetchAccountInfoRequest : public SeafileApiRequest
{
    Q_OBJECT
public:
    FetchAccountInfoRequest(const Account& account);

    const Account& account() const
    {
        return account_;
    }

signals:
    void success(const AccountInfo& info);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(FetchAccountInfoRequest);

    Account account_;
};

class PrivateShareRequest : public SeafileApiRequest
{
    Q_OBJECT
public:
    enum ShareOperation {
        ADD_SHARE,
        UPDATE_SHARE,
        REMOVE_SHARE,
    };
    PrivateShareRequest(const Account& account,
                        const QString& repo_id,
                        const QString& path,
                        const SeafileUser& user,
                        int group_id,
                        SharePermission permission,
                        ShareType share_type,
                        ShareOperation op);

    ShareOperation shareOperation() const
    {
        return share_operation_;
    }

    int groupId() const
    {
        return share_type_ == SHARE_TO_GROUP ? group_id_ : -1;
    };

    SeafileUser user() const
    {
        return share_type_ == SHARE_TO_USER ? user_ : SeafileUser();
    };

    SharePermission permission() const
    {
        return permission_;
    }

    ShareType shareType() const
    {
        return share_type_;
    }

signals:
    void success();

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(PrivateShareRequest);

    int group_id_;
    SeafileUser user_;
    SharePermission permission_;
    ShareType share_type_;
    ShareOperation share_operation_;
};

class GetPrivateShareItemsRequest : public SeafileApiRequest
{
    Q_OBJECT
public:
    GetPrivateShareItemsRequest(const Account& account,
                                const QString& repo_id,
                                const QString& path);

signals:
    void success(const QList<GroupShareInfo>&, const QList<UserShareInfo>&);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(GetPrivateShareItemsRequest);
};

class FetchGroupsAndContactsRequest : public SeafileApiRequest
{
    Q_OBJECT
public:
    FetchGroupsAndContactsRequest(const Account& account);

signals:
    void success(const QList<SeafileGroup>&, const QList<SeafileUser>&);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(FetchGroupsAndContactsRequest);
};


class RemoteWipeReportRequest : public SeafileApiRequest
{
    Q_OBJECT
public:
    RemoteWipeReportRequest(const Account& account);

signals:
    void success();

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(RemoteWipeReportRequest);
};

class SearchUsersRequest : public SeafileApiRequest
{
    Q_OBJECT
public:
    SearchUsersRequest(const Account& account, const QString& pattern);

    QString pattern() const { return pattern_; }

signals:
    void success(const QList<SeafileUser>& users);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(SearchUsersRequest);

    QString pattern_;
};

class FetchGroupsRequest : public SeafileApiRequest
{
    Q_OBJECT
public:
    FetchGroupsRequest(const Account& account);

signals:
    void success(const QList<SeafileGroup>&);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(FetchGroupsRequest);
};

class GetThumbnailRequest : public SeafileApiRequest
{
    Q_OBJECT
public:
    GetThumbnailRequest(const Account& account,
                        const QString& repo_id,
                        const QString& path,
			const QString& dirent_id,
                        uint size);

    const Account& account() const
    {
        return account_;
    }
    const QString& repoId() const
    {
        return repo_id_;
    }
    const QString& path() const
    {
        return path_;
    }
    const QString& direntId() const
    {
        return dirent_id_;
    }
    uint size() const
    {
        return size_;
    }
signals:
    void success(const QPixmap& thumbnail);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(GetThumbnailRequest);
    Account account_;
    QString repo_id_;
    QString path_;
    QString dirent_id_;
    uint size_;
};

class UnshareRepoRequest : public SeafileApiRequest
{
    Q_OBJECT
public:
    UnshareRepoRequest(const Account& account,
                       const QString& repo_id,
                       const QString& from_user);

    const QString& repoId() { return repo_id_; }

signals:
    void success();

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(UnshareRepoRequest);

    QString repo_id_;
};

struct UploadLinkInfo {
    QString username;
    QString repo_id;
    QString ctime;
    QString token;
    QString link;
    QString path;
};

class CreateFileUploadLinkRequest : public SeafileApiRequest
{
    Q_OBJECT
public:
    CreateFileUploadLinkRequest(const Account& account,
                         const QString& repo_id,
                         const QString& path,
                         const QString& password = QString());

signals:
    void success(const UploadLinkInfo& link_info);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(CreateFileUploadLinkRequest);
    QString repo_id_;
    QString path_;
    QString password_;
};

class GetUploadFileLinkRequest : public SeafileApiRequest
{
    Q_OBJECT
public:
    GetUploadFileLinkRequest(const QString& link);

signals:
    void success(const QString& url);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(GetUploadFileLinkRequest);
};

#endif // SEAFILE_CLIENT_API_REQUESTS_H
