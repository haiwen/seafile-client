#ifndef SEAFILE_CLIENT_NETWORK_MANAGER_H
#define SEAFILE_CLIENT_NETWORK_MANAGER_H

#include <QObject>
#include <vector>
#include <QNetworkReply>
#include <QMutex>

#include "utils/singleton.h"

class QNetworkAccessManager;
class QNetworkProxy;
class QTimer;

class NetworkManager : public QObject {
  Q_OBJECT
public:
    static NetworkManager* instance() {
        if (!instance_) {
            static NetworkManager singleton;
            instance_ = &singleton;
        }
        return instance_;
    }
    void addWatch(QNetworkAccessManager* manager);
    void applyProxy(const QNetworkProxy& proxy);
    void reapplyProxy();

    // retry only once
    bool shouldRetry(const QNetworkReply::NetworkError error) {
        if ((error == QNetworkReply::ProxyConnectionClosedError ||
             error == QNetworkReply::ProxyConnectionRefusedError ||
             error == QNetworkReply::ProxyNotFoundError ||
             error == QNetworkReply::ProxyTimeoutError ||
             error == QNetworkReply::UnknownProxyError) &&
            should_retry_) {
            NetworkManager::instance()->reapplyProxy();
            should_retry_ = false;
            return true;
        }
        return false;
    }

signals:
    void proxyChanged(const QNetworkProxy& proxy);

private slots:
    void onCleanup();

private:
    std::vector<QNetworkAccessManager*> managers_;
    NetworkManager();
    ~NetworkManager() {}
    NetworkManager(const NetworkManager&) /* = delete */ ;
    bool should_retry_;
    static NetworkManager* instance_;
};

// Check the network connection periodically, and reset the qt
// QNetworkAccessManager if the network is reconnected.
class NetworkStatusDetector: public QObject
{
    Q_OBJECT
    SINGLETON_DEFINE(NetworkStatusDetector)
public:
    ~NetworkStatusDetector();
    void start();
    void stop();
    void setNetworkFailure();
    void setNetworkSuccess();

public slots:
    void detect();

signals:
    void check();

private:
    NetworkStatusDetector();
    QMutex network_error_mutex_;

    QTimer *check_timer_;
    bool has_network_failure_;
};


#endif // SEAFILE_CLIENT_NETWORK_MANAGER_H
