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
        if (http_error_code_ == 443) {
            return QObject::tr("The storage quota has been used up");
        } else {
            return QObject::tr("Server Error");
        }
    default:
        // impossible
        break;
    }

    return "";
}

ApiError ApiError::NoError() {
    ApiError error;
    error.type_ = NOT_AN_ERROR;
    return error;
}

bool ApiError::operator==(const ApiError& other)
{
    if (type_ != other.type_) {
        return false;
    }

    bool same;
    switch (type_) {
    case NOT_AN_ERROR:
        same = true;
        break;
    case HTTP_ERROR:
        same = http_error_code_ == other.http_error_code_;
        break;
    case NETWORK_ERROR:
        same = network_error_ == other.network_error_;
        break;
    case SSL_ERROR:
        same = ssl_errors_ == other.ssl_errors_;
        break;
    }

    return same;
}
