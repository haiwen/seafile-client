#include <QtGlobal>
#include <QtNetwork>
#include <QUrlQuery>

#include "utils/utils.h"
#include "api-client.h"
#include "api-error.h"
#include "server-status-service.h"

#include "api-request.h"

SeafileApiRequest::SeafileApiRequest(const QUrl& url, Method method,
                                     const QString& token)
    : url_(url),
      method_(method),
      token_(token)
{
    api_client_ = new SeafileApiClient;
}

SeafileApiRequest::~SeafileApiRequest()
{
    if (api_client_) {
        api_client_->deleteLater();
        api_client_ = nullptr;
    }
}

void SeafileApiRequest::setUrlParam(const QString& name, const QString& value)
{
    params_[name] = value;
}

void SeafileApiRequest::setFormParam(const QString& name, const QString& value)
{
    if (method_ != METHOD_PUT && method_ != METHOD_POST) {
        qWarning("warning: calling setFormParam on a request with method %d\n", method_);
    }
    form_params_[name] = value;
}

void SeafileApiRequest::setUseCache(bool use_cache)
{
    api_client_->setUseCache(use_cache);
}

void SeafileApiRequest::setHeader(const QString& key, const QString& value)
{
    api_client_->setHeader(key, value);
}

void SeafileApiRequest::send()
{
    if (token_.size() > 0) {
        api_client_->setToken(token_);
    }

    if (!params_.isEmpty()) {
        url_ = ::includeQueryParams(url_, params_);
    }

    QByteArray post_data;

    switch (method_) {
    case METHOD_GET:
        api_client_->get(url_);
        break;
    case METHOD_DELETE:
        api_client_->deleteResource(url_);
        break;
    case METHOD_POST:
    case METHOD_PUT:
        post_data = ::buildFormData(form_params_);
        api_client_->post(url_, post_data, method_ == METHOD_PUT);
        break;
    default:
        qWarning("unknown method %d\n", method_);
        return;
    }

    connect(api_client_, SIGNAL(requestSuccess(QNetworkReply&)),
            this, SLOT(requestSuccess(QNetworkReply&)));

    connect(api_client_, SIGNAL(requestSuccess(QNetworkReply&)),
            this, SLOT(updateServerStatus()));

    connect(api_client_, SIGNAL(networkError(const QNetworkReply::NetworkError&, const QString&)),
            this, SLOT(onNetworkError(const QNetworkReply::NetworkError&, const QString&)));

    connect(api_client_, SIGNAL(requestFailed(int)),
            this, SLOT(onHttpError(int)));
}

void SeafileApiRequest::onHttpError(int code)
{
    emit failed(ApiError::fromHttpError(code));

    // Even though the http request fails, we still know the server can be
    // reached without network problems.
    ServerStatusService::instance()->updateOnSuccessfullRequest(url_);
}

void SeafileApiRequest::onNetworkError(const QNetworkReply::NetworkError& error, const QString& error_string)
{
    emit failed(ApiError::fromNetworkError(error, error_string));
    ServerStatusService::instance()->updateOnFailedRequest(url_);
}


json_t* SeafileApiRequest::parseJSON(QNetworkReply &reply, json_error_t *error)
{
    QByteArray raw = reply.readAll();
    //qWarning("\n%s\n", raw.data());
    json_t *root = json_loads(raw.data(), 0, error);
    return root;
}

const QNetworkReply* SeafileApiRequest::reply() const
{
    return api_client_->reply();
}

void SeafileApiRequest::updateServerStatus()
{
    ServerStatusService::instance()->updateOnSuccessfullRequest(url_);
}
