#ifndef SEAFILE_CLIENT_SEAHUB_MESSAGES_MONITOR_
#define SEAFILE_CLIENT_SEAHUB_MESSAGES_MONITOR_

#include <QObject>

class QTimer;

class GetUnseenSeahubMessagesRequest;

class SeahubMessagesMonitor : public QObject
{
    Q_OBJECT
public:
    SeahubMessagesMonitor(QObject *parent=0);

public slots:
    void refresh();

private slots:
    void onBtnClicked();
    void onRequestSuccess(int count);

private:
    void resetStatus();

    GetUnseenSeahubMessagesRequest *req_;
    CloudView *cloud_view_;

    QTimer *refresh_timer_;

    int unread_count_;
};

#endif // SEAFILE_CLIENT_SEAHUB_MESSAGES_MONITOR_
