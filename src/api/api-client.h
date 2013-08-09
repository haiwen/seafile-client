#ifndef SEAFILE_API_CLIENT_H
#define SEAFILE_API_CLIENT_H

#include <QString>
#include <QObject>

#include "account.h"
#include "seaf-repo.h"

class QNetworkAccessManager;
class QByteArray;
class QNetworkReply;

/**
 * SeafileApiClient handles the underlying api mechanism
 */
class SeafileApiClient : public QObject {
    Q_OBJECT

public:
    SeafileApiClient();
    ~SeafileApiClient();
    void setToken(const QString& token) { token_ = token; };
    void get(const QUrl& url);
    void post(const QUrl& url, const QByteArray& encodedParams);

signals:
    void requestSuccess(QNetworkReply& reply);
    void requestFailed(int code);

private slots:
    void httpRequestFinished();

private:
    Q_DISABLE_COPY(SeafileApiClient)

    static QNetworkAccessManager *na_mgr_;

    QString token_;

    QNetworkReply *reply_;
};

#endif  // SEAFILE_API_CLIENT_H
