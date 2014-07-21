#include <QtGlobal>
#include <QtNetwork>
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QUrlQuery>
#endif

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

void SeafileApiRequest::setParam(const QString& name, const QString& value)
{
    QPair<QString, QString> pair(name, value);
    params_.push_back(pair);
}

void SeafileApiRequest::send()
{
    if (token_.size() > 0) {
        api_client_->setToken(token_);
    }

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    QUrlQuery urlQuery;
    switch (method_) {
    case METHOD_GET:
        urlQuery.setQueryItems(params_);
        url_.setQuery(urlQuery);
        api_client_->get(url_);
        break;
    case METHOD_POST:
        for (int i = 0; i < params_.size(); i++) {
            QPair<QString, QString> pair = params_[i];
            urlQuery.addQueryItem(QUrl::toPercentEncoding(pair.first),
                                       QUrl::toPercentEncoding(pair.second));
        }
        api_client_->post(url_, urlQuery.query(QUrl::FullyEncoded).toUtf8());
        break;
    }
#else /* Qt 4.x */
    switch (method_) {
    case METHOD_GET:
        url_.setQueryItems(params_);
        api_client_->get(url_);
        break;
    case METHOD_POST:
        QUrl params;
        for (int i = 0; i < params_.size(); i++) {
            QPair<QString, QString> pair = params_[i];
            params.addEncodedQueryItem(QUrl::toPercentEncoding(pair.first),
                                       QUrl::toPercentEncoding(pair.second));
        }
        api_client_->post(url_, params.encodedQuery());
        break;
    }
#endif /* Qt 4.x */

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
