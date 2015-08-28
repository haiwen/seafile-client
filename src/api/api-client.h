#ifndef SEAFILE_API_CLIENT_H
#define SEAFILE_API_CLIENT_H

#include <QString>
#include <QObject>
#include <QNetworkReply>

#include "account.h"
#include "server-repo.h"

class QNetworkAccessManager;
class QSslError;

/**
 * SeafileApiClient handles the underlying api mechanism
 */
class SeafileApiClient : public QObject {
    Q_OBJECT

public:
    SeafileApiClient(QObject *parent=0);
    ~SeafileApiClient();
    void setToken(const QString& token) { token_ = token; };
    void get(const QUrl& url);
    void post(const QUrl& url, const QByteArray& body, bool is_put);
    void deleteResource(const QUrl& url);

signals:
    void requestSuccess(QNetworkReply& reply);
    void requestFailed(int code);
    void networkError(const QNetworkReply::NetworkError& error, const QString& error_string);
    void sslErrors(QNetworkReply *, const QList<QSslError>&);

private slots:
    void httpRequestFinished();
    void onSslErrors(const QList<QSslError>& errors);

private:
    Q_DISABLE_COPY(SeafileApiClient)

    bool handleHttpRedirect();
    bool handleRedirectForNonGetRequest();

    void resendRequest(const QUrl& url);

    static QNetworkAccessManager *na_mgr_;

    QString token_;

    QByteArray body_;

    QNetworkReply *reply_;

    int redirect_count_;
};

#endif  // SEAFILE_API_CLIENT_H
