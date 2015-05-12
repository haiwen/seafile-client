#include "account.h"
#include "utils/utils.h"
#include "api/requests.h"

Account::~Account()
{
    if (serverInfoRequest)
        serverInfoRequest->deleteLater();
}

ServerInfoRequest* Account::createServerInfoRequest()
{
    if (serverInfoRequest)
        serverInfoRequest->deleteLater();
    return serverInfoRequest = new ServerInfoRequest(*this);
}

QUrl Account::getAbsoluteUrl(const QString& relativeUrl) const
{
    return ::urlJoin(serverUrl, relativeUrl);
}

QString Account::getSignature() const
{
    if (!isValid()) {
        return "";
    }

    return ::md5(serverUrl.host() + username).left(7);
}
