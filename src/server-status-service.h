#ifndef SEAFILE_CLIENT_SERVER_STATUS_SERVICE_H
#define SEAFILE_CLIENT_SERVER_STATUS_SERVICE_H

#include <QObject>
#include <QList>
#include <QUrl>
#include <QHash>

#include "utils/singleton.h"

class QTimer;

class PingServerRequest;
class ApiError;

class ServerStatus {
public:
    ServerStatus() {}
    ServerStatus(const QUrl& url, bool connected):
        url(url),
        connected(connected) {}

    QUrl url;
    bool connected;
};

class ServerStatusService : public QObject
{
    Q_OBJECT
    SINGLETON_DEFINE(ServerStatusService)
public:
    void start();
    void stop();

    // accessors
    const QList<ServerStatus> statuses() const { return statuses_.values(); }

    bool allServersConnected() const;
    bool allServersDisconnected() const;

public slots:
    void refresh(bool only_refresh_unconnected=false);
    void refreshUnconnected() { refresh(true); }
    void updateOnSuccessfullRequest(const QUrl& url);
    void updateOnFailedRequest(const QUrl& url);

private slots:
    void onPingServerSuccess();
    void onPingServerFailed();

signals:
    void serverStatusChanged();

private:
    Q_DISABLE_COPY(ServerStatusService)
    ServerStatusService(QObject *parent=0);

    void pingServer(const QUrl& url);
    bool isServerConnected(const QUrl& url) const;
    void updateOnRequestFinished(const QUrl& url, bool no_network_error);

    QTimer *refresh_timer_;
    QTimer *refresh_unconnected_timer_;

    QHash<QString, ServerStatus> statuses_;
    QHash<QString, PingServerRequest *> requests_;
};


#endif // SEAFILE_CLIENT_SERVER_STATUS_SERVICE_H
