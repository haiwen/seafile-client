#ifndef SEAFILE_API_REQUEST_H
#define SEAFILE_API_REQUEST_H

#include <QObject>
#include <QUrl>
#include <QMap>
#include <jansson.h>

class QNetworkReply;
class SeafileApiClient;

/**
 * Abstract base class for all types of api requests
 */
class SeafileApiRequest : public QObject {
    Q_OBJECT

public:
    virtual ~SeafileApiRequest();

    void setParam(const QString& name, const QString& value);
    void send();

signals:
    void failed(int code);

protected slots:
    virtual void requestSuccess(QNetworkReply& reply) = 0;

protected:
    enum Method {
        METHOD_POST,
        METHOD_GET
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

private:
    Q_DISABLE_COPY(SeafileApiRequest)

    QUrl url_;
    QUrl params_;
    Method method_;
    QString token_;
    SeafileApiClient* api_client_;
};

#endif // SEAFILE_API_REQUEST_H
