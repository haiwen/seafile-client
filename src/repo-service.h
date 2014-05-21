#ifndef SEAFILE_CLIENT_REPO_SERVICE_H_
#define SEAFILE_CLIENT_REPO_SERVICE_H_

#include <vector>
#include <QObject>

#include "api/server-repo.h"

class QTimer;

class ApiError;
class ListReposRequest;

class RepoService : public QObject
{
    Q_OBJECT
public:
    static RepoService *instance();

    void start();

    const std::vector<ServerRepo>& serverRepos() const { return server_repos_; }

    ServerRepo getRepo(const QString& repo_id) const;

    void refresh(bool force);

public slots:
    void refresh();

private slots:
    void onRefreshSuccess(const std::vector<ServerRepo>& repos);
    void onRefreshFailed(const ApiError& error);

signals:
    void refreshSuccess(const std::vector<ServerRepo>& repos);
    void refreshFailed(const ApiError& error);

private:
    Q_DISABLE_COPY(RepoService)

    RepoService(QObject *parent=0);

    static RepoService *singleton_;

    ListReposRequest *list_repo_req_;

    std::vector<ServerRepo> server_repos_;

    QTimer *refresh_timer_;
    bool in_refresh_;
};

#endif // SEAFILE_CLIENT_REPO_SERVICE_H_
