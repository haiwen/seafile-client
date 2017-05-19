#ifndef SEAFILE_API_REQUEST_H
#define SEAFILE_API_REQUEST_H

#include <jansson.h>

#include <QObject>
#include <QUrl>
#include <QMap>
#include <QPair>
#include <QList>
#include <QNetworkReply>
#include <QHash>

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

    const QUrl& url() const { return url_; }

    // set param k-v pair which appears in query params
    void setUrlParam(const QString& name, const QString& value);
    // set param k-v pair which appears in url-encoded form
    void setFormParam(const QString& name, const QString& value);
    // useful for static resources like images
    void setUseCache(bool use_cache);

    void setHeader(const QString& key, const QString& value);

    virtual void send();

    const QNetworkReply* reply() const;

signals:
    void failed(const ApiError& error);

protected slots:
    virtual void requestSuccess(QNetworkReply& reply) = 0;
    void onNetworkError(const QNetworkReply::NetworkError& error, const QString& error_string);
    void onHttpError(int);

protected:
    enum Method {
        // post action, passing urlParam and formParam
        METHOD_POST,
        // get action, passing urlParam only
        METHOD_GET,
        // put action, passing urlParam and formParam
        METHOD_PUT,
        // delete action, passing urlParam only
        METHOD_DELETE
    };

    SeafileApiRequest(const QUrl& url,
                      const Method method,
                      const QString& token = QString());

    json_t* parseJSON(QNetworkReply &reply, json_error_t *error);

    // Used with QScopedPointer for json_t
    struct JsonPointerCustomDeleter {
        static inline void cleanup(json_t *json) {
            json_decref(json);
        }
    };

private slots:
    void updateServerStatus();

private:
    Q_DISABLE_COPY(SeafileApiRequest)

    QUrl url_;
    QHash<QString, QString> params_;
    QHash<QString, QString> form_params_;
    Method method_;
    QString token_;
    SeafileApiClient* api_client_;
};

#endif // SEAFILE_API_REQUEST_H
