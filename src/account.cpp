#include "account.h"
#include "utils/utils.h"

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
