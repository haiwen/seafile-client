#ifndef SEAFILE_CLIENT_CUSTOMIAZATION_SERVICE_H
#define SEAFILE_CLIENT_CUSTOMIAZATION_SERVICE_H

#include <QObject>
#include <QUrl>
#include <QPixmap>
#include <QHash>

#include "utils/singleton.h"
#include "account.h"

class QNetworkDiskCache;

class FetchCustomLogoRequest;
class ApiError;

class CustomizationService : public QObject
{
    Q_OBJECT
    SINGLETON_DEFINE(CustomizationService)
public:
    void start();

    // Read the logo from the disk cache if exists, otherwise return a default
    // logo and send a request to fetch the logo.
    QPixmap getServerLogo(const Account& account);

    QNetworkDiskCache *diskCache() const { return disk_cache_; }

signals:
    void serverLogoFetched(const QUrl& url);

private slots:
    void onServerLogoFetched(const QUrl& url);
    void onServerLogoFetchFailed(const ApiError& error);

private:
    Q_DISABLE_COPY(CustomizationService)
    CustomizationService(QObject* parent = 0);

    void cleanUpRequest(FetchCustomLogoRequest *req);

    QHash<QString, FetchCustomLogoRequest*> reqs_;

    QNetworkDiskCache* disk_cache_;
};


#endif // SEAFILE_CLIENT_CUSTOMIAZATION_SERVICE_H
