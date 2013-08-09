#ifndef _SEAF_CONNECTION_H
#define _SEAF_CONNECTION_H

#include <QString>
#include <QNetworkReply>

#include "account.h"

class QNetworkAccessManager;

class SeafConnection : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY(SeafConnection)
    SeafConnection();
public:
    static SeafConnection* instance();
    void accountLogin(const QUrl& serverAddr, const QString& username, const QString& password);

signals:
    void accountLoginSuccess(const Account& account);
    void accountLoginFailed();

private slots:
    void loginRequestFinished();

private:
    static SeafConnection *singleton_;

    Account current_login_account_;
    
    QNetworkAccessManager *network_access_mgr_;
    
    QNetworkReply *current_login_request_;
};

#endif  // _SEAF_CONNECTION_H
