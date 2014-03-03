#include "api-error.h"

ApiError::ApiError()
{
    ssl_reply_ = NULL;
}

ApiError ApiError::fromNetworkError(const QNetworkReply::NetworkError& network_error,
                                    const QString& network_error_string)
{
    ApiError error;

    error.type_ = NETWORK_ERROR;
    error.network_error_ = network_error;
    error.network_error_string_ = network_error_string;

    return error;
}

ApiError ApiError::fromSslErrors(QNetworkReply *reply, const QList<QSslError>& ssl_errors)
{
    ApiError error;

    error.type_ = SSL_ERROR;
    error.ssl_reply_ = reply;
    error.ssl_errors_ = ssl_errors;

    return error;
}

ApiError ApiError::fromHttpError(int code)
{
    ApiError error;

    error.type_ = HTTP_ERROR;
    error.http_error_code_ = code;

    return error;
}

ApiError ApiError::fromJsonError()
{
    ApiError error;

    error.type_ = HTTP_ERROR;
    // 500 internal server error
    error.http_error_code_ = 500;

    return error;
}

QString ApiError::toString() const {
    switch (type_) {
    case SSL_ERROR:
        return QObject::tr("SSL Error");
    case NETWORK_ERROR:
        return QObject::tr("Network Error: %1").arg(network_error_string_);
    case HTTP_ERROR:
        return QObject::tr("Server Error");
    default:
        // impossible
        break;
    }

    return "";
}
