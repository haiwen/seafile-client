#ifndef SEAFILE_CLIENT_SEAHUB_MESSAGES_MONITOR_
#define SEAFILE_CLIENT_SEAHUB_MESSAGES_MONITOR_

#include <QObject>

class QToolButton;
class QTimer;

class CloudView;
class GetUnseenSeahubMessagesRequest;

class SeahubMessagesMonitor : public QObject
{
    Q_OBJECT
public:
    SeahubMessagesMonitor(CloudView *cloud_view, QObject *parent=0);

public slots:
    void refresh();

private slots:
    void onBtnClicked();
    void onRequestSuccess(int count);

private:
    void resetStatus();

    GetUnseenSeahubMessagesRequest *req_;
    CloudView *cloud_view_;

    QToolButton *btn_;
    QTimer *refresh_timer_;

    int unread_count_;
};

#endif // SEAFILE_CLIENT_SEAHUB_MESSAGES_MONITOR_
