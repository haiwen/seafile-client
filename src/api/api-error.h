#ifndef SEAFILE_API_ERROR_H
#define SEAFILE_API_ERROR_H

#include <QString>
#include <QNetworkReply>
#include <QList>
#include <QSslError>

#include "server-repo.h"

class QNetworkAccessManager;
class QByteArray;

/**
 * Error info in an api request.
 */
class ApiError {
public:
    enum ErrorType {
        // network error, like connection refused
        NETWORK_ERROR,
        // ssl error, like invalid ssl certificate
        SSL_ERROR,
        // http error, like 4xx, 5xx
        HTTP_ERROR,
    };

    static ApiError fromNetworkError(const QNetworkReply::NetworkError& error, const QString& error_string);
    static ApiError fromSslErrors(QNetworkReply *reply, const QList<QSslError>& errors);
    static ApiError fromHttpError(int code);
    static ApiError fromJsonError();

    // accessors
    ErrorType type() const { return type_; }

    const QNetworkReply::NetworkError& networkError() const { return network_error_; }
    const QString& networkErrorString() const { return network_error_string_; }

    QNetworkReply *sslReply() const { return ssl_reply_; }
    const QList<QSslError>& sslErrors() const { return ssl_errors_; }

    int httpErrorCode() const { return http_error_code_; }

    QString toString() const;

private:
    ApiError();

    ErrorType type_;

    int http_error_code_;

    QNetworkReply::NetworkError network_error_;
    QString network_error_string_;

    QNetworkReply *ssl_reply_;
    QList<QSslError> ssl_errors_;
};

#endif  // SEAFILE_API_ERROR_H
