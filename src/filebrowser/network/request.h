#ifndef SEAFILE_CLIENT_NETWORK_REQUEST_H
#define SEAFILE_CLIENT_NETWORK_REQUEST_H
#include <QtGlobal>
#include <QNetworkRequest>

class QUrl;

class SeafileNetworkRequest : public QNetworkRequest {
public:
    SeafileNetworkRequest(const SeafileNetworkRequest &req);
    SeafileNetworkRequest(const QString &token, const QUrl &url);
    SeafileNetworkRequest(const QString &token, const QNetworkRequest &req);

    QString token() const { return token_; }
    void setToken(const QString &token);

private:
    QByteArray token_;
};

#endif  // SEAFILE_CLIENT_NETWORK_REQUEST_H
