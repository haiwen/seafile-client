#ifndef SEAFILE_CLIENT_SERVER_STATUS_DIALOG_H
#define SEAFILE_CLIENT_SERVER_STATUS_DIALOG_H

#include <QDialog>
#include "ui_server-status-dialog.h"

class QTimer;

class ServerStatusDialog : public QDialog,
                           public Ui::ServerStatusDialog
{
    Q_OBJECT
public:
    ServerStatusDialog(QWidget *parent=0);

private slots:
    void refreshStatus();

private:
    Q_DISABLE_COPY(ServerStatusDialog)

    QTimer *refresh_timer_;
};

#endif // SEAFILE_CLIENT_SERVER_STATUS_DIALOG_H
