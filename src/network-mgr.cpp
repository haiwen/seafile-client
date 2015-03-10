#include "network-mgr.h"
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <algorithm>
namespace {
QNetworkProxy proxy_;
} // anonymous namespace

NetworkManager* NetworkManager::instance_ = NULL;

void NetworkManager::addWatch(QNetworkAccessManager* manager)
{
    if (std::find(managers_.begin(), managers_.end(), manager) == managers_.end()) {
        connect(manager, SIGNAL(destroyed()), this, SLOT(onCleanup()));
        managers_.push_back(manager);
    }
}

void NetworkManager::applyProxy(const QNetworkProxy& proxy)
{
    proxy_ = proxy;
    should_retry_ = true;
    QNetworkProxy::setApplicationProxy(proxy_);
    for(std::vector<QNetworkAccessManager*>::iterator pos = managers_.begin();
        pos != managers_.end(); ++pos)
        (*pos)->setProxy(proxy_);
    emit proxyChanged(proxy_);
}

void NetworkManager::reapplyProxy()
{
    applyProxy(proxy_);
}

void NetworkManager::onCleanup()
{
    QNetworkAccessManager *manager = qobject_cast<QNetworkAccessManager*>(sender());
    if (manager) {
        managers_.erase(std::remove(managers_.begin(), managers_.end(), manager),
                        managers_.end());
    }
}
