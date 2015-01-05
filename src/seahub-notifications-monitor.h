#ifndef SEAFILE_CLIENT_SEAHUB_MESSAGES_MONITOR_
#define SEAFILE_CLIENT_SEAHUB_MESSAGES_MONITOR_

#include <QObject>

class QTimer;

class ApiError;
class GetUnseenSeahubNotificationsRequest;

class SeahubNotificationsMonitor : public QObject
{
    Q_OBJECT
public:
    static SeahubNotificationsMonitor* instance();

    void start();
    void refresh(bool force);

    int getUnreadNotifications() const { return unread_count_; }

    void openNotificationsPageInBrowser();

public slots:
    void refresh();

signals:
    void notificationsChanged();

private slots:
    void onRequestSuccess(int count);
    void onRequestFailed(const ApiError& error);
    void onAccountChanged();

private:
    SeahubNotificationsMonitor(QObject *parent=0);
    static SeahubNotificationsMonitor *singleton_;

    void resetStatus();
    void setUnreadNotificationsCount(int count);

    QTimer *refresh_timer_;
    GetUnseenSeahubNotificationsRequest *check_messages_req_;
    bool in_refresh_;

    int unread_count_;
};

#endif // SEAFILE_CLIENT_SEAHUB_MESSAGES_MONITOR_
