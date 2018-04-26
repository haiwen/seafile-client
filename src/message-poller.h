#ifndef SEAFILE_CLIENT_MESSAGE_POLLER_H
#define SEAFILE_CLIENT_MESSAGE_POLLER_H

#include <QObject>

class QTimer;
class SeafileRpcClient;

struct SyncNotification;

class MessagePoller : public QObject {
    Q_OBJECT
public:
    MessagePoller(QObject *parent=0);
    ~MessagePoller();

    void start();

signals:
    void seadriveFSLoaded();

private slots:
    void checkNotification();

private:
    Q_DISABLE_COPY(MessagePoller)

    void processNotification(const SyncNotification& notification);

    SeafileRpcClient *rpc_client_;

    QTimer *check_notification_timer_;
};

#endif // SEAFILE_CLIENT_MESSAGE_POLLER_H
