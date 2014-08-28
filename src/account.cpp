#include "account.h"
#include "utils/utils.h"

QUrl Account::getAbsoluteUrl(const QString& relativeUrl) const
{
    return ::urlJoin(serverUrl, relativeUrl);
}
