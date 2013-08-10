#ifndef SEAFILE_LIST_REPOS_REQUEST_H
#define SEAFILE_LIST_REPOS_REQUEST_H

#include <vector>
#include "api-request.h"
#include "seaf-repo.h"

class QNetworkReply;
class Account;

class ListReposRequest : public SeafileApiRequest {
    Q_OBJECT

public:
    ListReposRequest(const Account& account);

protected slots:
    void requestSuccess(QNetworkReply& reply);

signals:
    void success(const std::vector<SeafRepo>& repos);

private:
    Q_DISABLE_COPY(ListReposRequest)
};

#endif // SEAFILE_LIST_REPOS_REQUEST_H
