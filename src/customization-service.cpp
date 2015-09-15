#include <QTimer>
#include <QDir>
#include <QNetworkDiskCache>

#include "customization-service.h"
#include "seafile-applet.h"
#include "account-mgr.h"
#include "configurator.h"
#include "api/api-error.h"
#include "api/requests.h"


SINGLETON_IMPL(CustomizationService)

CustomizationService::CustomizationService(QObject* parent) : QObject(parent)
{
    disk_cache_ = new QNetworkDiskCache();
}

void CustomizationService::start()
{
    disk_cache_->setCacheDirectory(
        QDir(seafApplet->configurator()->seafileDir())
            .filePath("customization"));
}

QPixmap CustomizationService::getServerLogo(const Account& account)
{
    QPixmap logo = QPixmap(":/images/seafile-24.png");
    if (account.serverInfo.customLogo.isEmpty()) {
        return logo;
    }

    QUrl url = account.getAbsoluteUrl(account.serverInfo.customLogo);
    QIODevice* buf = disk_cache_->data(url);
    if (buf) {
        logo.loadFromData(buf->readAll());
        buf->close();
        delete buf;
    }

    if (!reqs_.contains(url.toString())) {
        FetchCustomLogoRequest* req = new FetchCustomLogoRequest(url);
        connect(req,
                SIGNAL(success(const QUrl&)),
                this,
                SLOT(onServerLogoFetched(const QUrl&)));
        connect(req,
                SIGNAL(failed(const ApiError&)),
                this,
                SLOT(onServerLogoFetchFailed(const ApiError&)));

        req->send();
        reqs_[url.toString()] = req;
    }

    return logo;
}

void CustomizationService::onServerLogoFetched(const QUrl& url)
{
    emit(serverLogoFetched(url));
    FetchCustomLogoRequest* req =
        qobject_cast<FetchCustomLogoRequest*>(sender());
    cleanUpRequest(req);
}

void CustomizationService::onServerLogoFetchFailed(const ApiError& error)
{
    FetchCustomLogoRequest* req =
        qobject_cast<FetchCustomLogoRequest*>(sender());
    cleanUpRequest(req);
}

void CustomizationService::cleanUpRequest(FetchCustomLogoRequest* req)
{
    reqs_.remove(req->url().toString());
    req->deleteLater();
}
