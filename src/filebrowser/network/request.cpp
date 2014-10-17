#include "request.h"

SeafileNetworkRequest::SeafileNetworkRequest(const QString &token,
                                       const QUrl &url)
    : QNetworkRequest(url)
{
    token_ = token.toUtf8();
    setRawHeader("Authorization", "Token " + token_);
}

SeafileNetworkRequest::SeafileNetworkRequest(const QString &token,
                                       const QNetworkRequest &req)
    : QNetworkRequest(req)
{
    token_ = token.toUtf8();
    setRawHeader("Authorization", "Token " + token_);
}

SeafileNetworkRequest::SeafileNetworkRequest(const SeafileNetworkRequest &req)
    : QNetworkRequest(req)
{
    token_ = req.token_;
    setRawHeader("Authorization", "Token " + token_);
}

void SeafileNetworkRequest::setToken(const QString &token)
{
    token_ = token.toUtf8();
    setRawHeader("Authorization", "Token " + token_);
}
