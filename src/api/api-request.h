#ifndef SEAFILE_API_REQUEST_H
#define SEAFILE_API_REQUEST_H

#include <QObject>
#include <QUrl>
#include <QMap>
#include <QPair>
#include <QList>
#include <jansson.h>
#include <QNetworkReply>

class SeafileApiClient;
class QSslError;
class ApiError;

/**
 * Abstract base class for all types of api requests
 */
class SeafileApiRequest : public QObject {
    Q_OBJECT

public:
    virtual ~SeafileApiRequest();

    void setParam(const QString& name, const QString& value);
    void send();
    void setIgnoreSslErrors(bool ignore) { ignore_ssl_errors_ = ignore; }

signals:
    void failed(const ApiError& error);

protected slots:
    virtual void requestSuccess(QNetworkReply& reply) = 0;
    void onSslErrors(QNetworkReply *reply, const QList<QSslError>& errors);
    void onNetworkError(const QNetworkReply::NetworkError& error, const QString& error_string);
    void onHttpError(int);

protected:
    enum Method {
        METHOD_POST,
        METHOD_GET
    };

    SeafileApiRequest(const QUrl& url,
                      const Method method,
                      const QString& token = QString(),
                      bool ignore_ssl_errors_=true);

    json_t* parseJSON(QNetworkReply &reply, json_error_t *error);

    // Used with QScopedPointer for json_t
    struct JsonPointerCustomDeleter {
        static inline void cleanup(json_t *json) {
            json_decref(json);
        }
    };

protected:
    const QUrl& url() const { return url_; }

private:
    Q_DISABLE_COPY(SeafileApiRequest)

    QUrl url_;
    QList<QPair<QString, QString> > params_;
    Method method_;
    QString token_;
    SeafileApiClient* api_client_;

    bool ignore_ssl_errors_;
};

#endif // SEAFILE_API_REQUEST_H
