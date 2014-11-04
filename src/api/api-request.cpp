#include <QtNetwork>

#include "utils/utils.h"
#include "api-client.h"
#include "api-error.h"

#include "api-request.h"

SeafileApiRequest::SeafileApiRequest(const QUrl& url, Method method,
                                     const QString& token, bool ignore_ssl_errors)
    : url_(url),
      method_(method),
      token_(token),
      ignore_ssl_errors_(ignore_ssl_errors)
{
    api_client_ = new SeafileApiClient;
}

SeafileApiRequest::~SeafileApiRequest()
{
    delete api_client_;
}

void SeafileApiRequest::setUrlParam(const QString& name, const QString& value)
{
    params_.push_back(QPair<QByteArray, QByteArray>(
            QUrl::toPercentEncoding(name), QUrl::toPercentEncoding(value)));
}

void SeafileApiRequest::setFormParam(const QString& name, const QString& value)
{
    form_params_.push_back(QPair<QByteArray, QByteArray>(
            QUrl::toPercentEncoding(name), QUrl::toPercentEncoding(value)));
}

void SeafileApiRequest::send()
{
    if (token_.size() > 0) {
        api_client_->setToken(token_);
    }

    switch (method_) {
    case METHOD_GET:
        url_.setEncodedQueryItems(params_);
        api_client_->get(url_);
        break;
    case METHOD_DELETE:
        url_.setEncodedQueryItems(params_);
        api_client_->deleteResource(url_);
        break;
    case METHOD_POST:
        url_.setEncodedQueryItems(params_);
        if (!form_params_.isEmpty()) {
            QUrl params;
            params.setEncodedQueryItems(form_params_);
            setData(params.encodedQuery());
        }
        api_client_->post(url_, data_);
        break;
    case METHOD_PUT:
        url_.setEncodedQueryItems(params_);
        if (!form_params_.isEmpty()) {
            QUrl params;
            params.setEncodedQueryItems(form_params_);
            setData(params.encodedQuery());
        }
        api_client_->post(url_, data_, true);
        break;
    default:
        qWarning("unknown method %d\n", method_);
        return;
    }

    connect(api_client_, SIGNAL(requestSuccess(QNetworkReply&)),
            this, SLOT(requestSuccess(QNetworkReply&)));

    connect(api_client_, SIGNAL(networkError(const QNetworkReply::NetworkError&, const QString&)),
            this, SLOT(onNetworkError(const QNetworkReply::NetworkError&, const QString&)));

    connect(api_client_, SIGNAL(requestFailed(int)),
            this, SLOT(onHttpError(int)));

    connect(api_client_, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError>&)),
            this, SLOT(onSslErrors(QNetworkReply*, const QList<QSslError>&)));

}

void SeafileApiRequest::onHttpError(int code)
{
    emit failed(ApiError::fromHttpError(code));
}

void SeafileApiRequest::onNetworkError(const QNetworkReply::NetworkError& error, const QString& error_string)
{
    emit failed(ApiError::fromNetworkError(error, error_string));
}

void SeafileApiRequest::onSslErrors(QNetworkReply* reply, const QList<QSslError>& errors)
{
    if (ignore_ssl_errors_) {
        reply->ignoreSslErrors();
    } else {
        emit failed(ApiError::fromSslErrors(reply, errors));
    }
}

json_t* SeafileApiRequest::parseJSON(QNetworkReply &reply, json_error_t *error)
{
    QByteArray raw = reply.readAll();
    //qDebug("\n%s\n", raw.data());
    json_t *root = json_loads(raw.data(), 0, error);
    return root;
}
