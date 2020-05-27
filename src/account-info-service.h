#ifndef SEAFILE_CLIENT_ACCOUNT_INFO_SERVICE_H
#define SEAFILE_CLIENT_ACCOUNT_INFO_SERVICE_H

#include <QObject>
#include <QList>
#include <QUrl>
#include <QHash>

#include "utils/singleton.h"

class QTimer;

class FetchAccountInfoRequest;
class ApiError;
class AccountInfo;


class AccountInfoService : public QObject
{
    Q_OBJECT
    SINGLETON_DEFINE(AccountInfoService)

public:
    void start();
    void stop();

public slots:               
    void refresh();

private slots:
    void onFetchAccountInfoSuccess(const AccountInfo& info);
    void onFetchAccountInfoFailed();

private:
    Q_DISABLE_COPY(AccountInfoService)
    AccountInfoService(QObject *parent=0);

    QTimer *refresh_timer_;
};


#endif // SEAFILE_CLIENT_ACCOUNT_INFO_SERVICE_H
