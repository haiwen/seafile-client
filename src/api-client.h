#ifndef SEAFILE_API_CLIENT_H
#define SEAFILE_API_CLIENT_H

#include <vector>
#include <QString>
#include <QNetworkReply>

#include "account.h"
#include "seaf-repo.h"

class QNetworkAccessManager;

class SeafileApiClient : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY(SeafileApiClient)
    SeafileApiClient();

public:
    static SeafileApiClient* instance();
    void accountLogin(const QUrl& serverAddr, const QString& username, const QString& password);
    void getAccountRepos(const Account& account);

signals:
    void accountLoginSuccess(const Account& account);
    void accountLoginFailed();

    void getAccountReposSuccess(const std::vector<SeafRepo>& repos);
    void getAccountReposFailed();

private slots:
    void loginRequestFinished();

private:
    static SeafileApiClient *singleton_;

    Account current_login_account_;

    QNetworkAccessManager *network_access_mgr_;

    QNetworkReply *current_login_request_;
};

#endif  // SEAFILE_API_CLIENT_H
