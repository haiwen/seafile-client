#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <algorithm>
#include <QStringRef>
#include <QSslConfiguration>
#include <QSslSocket>
#include <QSslCipher>
#include <QTimer>
#include <QThread>
#include <QApplication>
#include <QNetworkConfigurationManager>

#include "utils/utils-mac.h"
#include "api/api-client.h"
#include "network-mgr.h"

namespace {

const int kCheckNetworkStatusIntervalMSecs = 10000; // 10s

QNetworkProxy proxy_;

template <class T, std::size_t N>
inline size_t ArrayLengthOf(T (&)[N]) {
  return N;
}

const char *const kWhitelistCiphers[] = {
    "ECDHE-RSA-AES256-GCM-SHA384"
    "ECDHE-RSA-AES128-GCM-SHA256"
    "DHE-RSA-AES256-GCM-SHA384"
    "DHE-RSA-AES128-GCM-SHA256"
    "ECDHE-RSA-AES256-SHA384"
    "ECDHE-RSA-AES128-SHA256"
    "ECDHE-RSA-AES256-SHA"
    "ECDHE-RSA-AES128-SHA"
    "DHE-RSA-AES256-SHA256"
    "DHE-RSA-AES128-SHA256"
    "DHE-RSA-AES256-SHA"
    "DHE-RSA-AES128-SHA"
    "ECDHE-RSA-DES-CBC3-SHA"
    "EDH-RSA-DES-CBC3-SHA"
    "AES256-GCM-SHA384"
    "AES128-GCM-SHA256"
    "AES256-SHA256"
    "AES128-SHA256"
    "AES256-SHA"
    "AES128-SHA"
    "DES-CBC3-SHA"
};

// return false if it contains RC4, RSK, CBC, MD5, DES, DSS, EXPORT, NULL
bool isWeakCipher(const QString& cipher_name)
{
    int current_begin = 0;
    int current_end;
    QStringRef name;
    while((current_end = cipher_name.indexOf("-", current_begin)) != -1) {
        name = QStringRef(&cipher_name, current_begin, current_end - current_begin);
        if (name == "RC4")
            return true;
        else if (name == "PSK")
            return true;
        else if (name == "CBC")
            return true;
        else if (name == "MD5")
            return true;
        else if (name == "DES")
            return true;
        else if (name == "DSS")
            return true;
        else if (name == "EXP")
            return true;
        else if (name == "NULL")
            return true;

        current_begin = current_end + 1;
    }
    return false;
}

void disableWeakCiphers()
{
    QSslConfiguration configuration = QSslConfiguration::defaultConfiguration();
    const QList<QSslCipher> ciphers = QSslSocket::supportedCiphers();

    QList<QSslCipher> new_ciphers;
    Q_FOREACH(const QSslCipher &cipher, ciphers)
    {
        bool whitelisted = false;
        for (unsigned i = 0; i < ArrayLengthOf(kWhitelistCiphers); ++i) {
            if (cipher.name() == kWhitelistCiphers[i]) {
                whitelisted = true;
                break;
            }
        }
        if (!whitelisted) {
            // blacklist eNULL, no encryption
            if (cipher.encryptionMethod().isEmpty())
                continue;
            // blacklist aNULL, no authentication
            if (cipher.authenticationMethod().isEmpty())
                continue;
            // blacklist RC4, RSK, CBC, MD5, DES, DSS, EXPORT, NULL
            const QString cipher_name = cipher.name();
            if (isWeakCipher(cipher_name))
                continue;
        }
        new_ciphers.push_back(cipher);
    }
    configuration.setCiphers(new_ciphers);
    QSslConfiguration::setDefaultConfiguration(configuration);
}

#ifdef Q_OS_MAC
void loadUserCaCertificate()
{
    QList<QSslCertificate> certificates;
    const std::vector<QByteArray> certs = utils::mac::getSystemCaCertificates();
    for (unsigned i = 0; i < certs.size(); ++i)
    {
        certificates.append(QSslCertificate::fromData(certs[i], QSsl::Der));
    }

    // remove duplicates
    certificates = certificates.toSet().toList();

    QSslSocket::setDefaultCaCertificates(certificates);
}
#endif
} // anonymous namespace

NetworkManager* NetworkManager::instance_ = NULL;

NetworkManager::NetworkManager() : should_retry_(true) {
    // remove unsafe cipher
    disableWeakCiphers();

#ifdef Q_OS_MAC
    // load user ca certificate from system, mac only
    loadUserCaCertificate();
#endif
}

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
    // Don't use "qobject_cast<QNetworkAccessManager>" here, because the
    // "destroyed" signal is emited by QObject class, so qobject_cast would fail
    // to cast it to QNetworkAccessManager.
    QNetworkAccessManager *manager = (QNetworkAccessManager*)(sender());
    managers_.erase(std::remove(managers_.begin(), managers_.end(), manager),
                    managers_.end());
}

SINGLETON_IMPL(NetworkStatusDetector)

NetworkStatusDetector::NetworkStatusDetector() {
    last_connected_ = true;

    check_timer_ = new QTimer(this);

    worker_thread_ = new QThread;
    NetworkCheckWorker *worker = new NetworkCheckWorker;
    worker->moveToThread(worker_thread_);

    connect(worker_thread_, &QThread::finished, worker, &QObject::deleteLater);
    connect(check_timer_, &QTimer::timeout, worker, &NetworkCheckWorker::check);
    connect(worker, &NetworkCheckWorker::resultReady, this, &NetworkStatusDetector::handleResults);
}

NetworkStatusDetector::~NetworkStatusDetector() {
    stop();
}

void NetworkStatusDetector::start() {
    qWarning("Starting the network status detector");
    worker_thread_->start();
    check_timer_->start(kCheckNetworkStatusIntervalMSecs);
}

void NetworkStatusDetector::stop() {
    check_timer_->stop();
    worker_thread_->quit();
    worker_thread_->wait();
}


void NetworkStatusDetector::handleResults(bool connected)
{
    qDebug("current network status is: %s", connected ? "connected" : "disconnected");
    if (last_connected_ == connected) {
        return;
    }

    if (last_connected_ && !connected) {
        qWarning("[network check] network is disconnected");
    } else if (!last_connected_ && connected) {
        qWarning("[network check] network is connected again, resetting qt network access manager");
        SeafileApiClient::resetQNAM();
    }

    last_connected_ = connected;
}

NetworkCheckWorker::NetworkCheckWorker()
{
    qnam_ = nullptr;
    reset();
}

NetworkCheckWorker::~NetworkCheckWorker() {
    if (qnam_) {
        qnam_->deleteLater();
    }
    qnam_ = nullptr;
}

void NetworkCheckWorker::reset()
{
    if (qnam_) {
        qnam_->deleteLater();
    }
    qnam_ = new QNetworkAccessManager(this);

    // TODO: do we need to add this qnam to watch? Note: NetworkManager::instance belongs to the main thread?
    // NetworkManager::instance()->addWatch(qnam_);

    // From: http://www.qtcentre.org/threads/37514-use-of-QNetworkAccessManager-networkAccessible
    //
    // QNetworkAccessManager::networkAccessible is not explicitly set when the
    // QNetworkAccessManager is created. It is only set after the network
    // session is initialized. The session is initialized automatically when you
    // make a network request or you can initialize it before hand with
    // QNetworkAccessManager::setConfiguration() or the
    // QNetworkConfigurationManager::NetworkSessionRequired flag is set.
    QNetworkConfigurationManager manager;
    qnam_->setConfiguration(manager.defaultConfiguration());
}

void NetworkCheckWorker::check()
{
    if (!qnam_) {
        return;
    }

    bool connected = qnam_->networkAccessible() == QNetworkAccessManager::Accessible;
    if (!connected) {
        reset();
    }
    emit resultReady(connected);
}
