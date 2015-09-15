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
    QPixmap default_logo = QPixmap(":/images/seafile-24.png");
    if (account.serverInfo.customLogo.isEmpty()) {
        return default_logo;
    }

    QUrl url = account.getAbsoluteUrl(account.serverInfo.customLogo);
    QIODevice* buf = disk_cache_->data(url);
    if (buf) {
        QPixmap logo;
        logo.loadFromData(buf->readAll());
        buf->close();
        delete buf;
        return logo;
    }

    if (!reqs_.contains(url.toString())) {
        FetchCustomLogoRequest* req = new FetchCustomLogoRequest(url);
        connect(req, SIGNAL(success()), this, SLOT(onServerLogoFetched(url)));
        connect(req,
                SIGNAL(failed(const ApiError&)),
                this,
                SLOT(onServerLogoFetchFailed()));

        req->send();
        reqs_[url.toString()] = req;
    }

    return default_logo;
}

void CustomizationService::onServerLogoFetched(const QUrl& url)
{
    emit(serverLogoFetched(url));
    FetchCustomLogoRequest* req =
        qobject_cast<FetchCustomLogoRequest*>(sender());
    req->deleteLater();
}

void CustomizationService::onServerLogoFetchFailed(const ApiError& error)
{
    FetchCustomLogoRequest* req =
        qobject_cast<FetchCustomLogoRequest*>(sender());
    req->deleteLater();
}
