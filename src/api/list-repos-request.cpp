#include <QtNetwork>
#include <jansson.h>
#include <QScopedPointer>

#include "list-repos-request.h"
#include "account.h"

namespace {

const char *kListReposUrl = "/api2/repos/";

} // namespace

ListReposRequest::ListReposRequest(const Account& account)
    : SeafileApiRequest (QUrl(account.serverUrl.toString() + kListReposUrl),
                         SeafileApiRequest::METHOD_GET, account.token)
{
}

void ListReposRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t *root = parseJSON(reply, &error);
    if (!root) {
        qDebug("failed to parse json:%s\n", error.text);
        emit failed(0);
        return;
    }

    QScopedPointer<json_t, JsonPointerCustomDeleter> json(root);

    std::vector<SeafRepo> repos = SeafRepo::listFromJSON(json.data(), &error);
    emit success(repos);
}
