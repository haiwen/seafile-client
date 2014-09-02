#ifndef SEAFILE_CLIENT_UI_SSL_CONFIRM_DIALOG_H
#define SEAFILE_CLIENT_UI_SSL_CONFIRM_DIALOG_H

#include <QUrl>

#include <QDialog>
#include "ui_ssl-confirm-dialog.h"

class SslConfirmDialog : public QDialog,
                         public Ui::SslConfirmDialog
{
    Q_OBJECT
public:
    SslConfirmDialog(const QUrl& url,
                     const QString fingerprint,
                     const QString prev_fingerprint = QString(""),
                     QWidget *parent=0);

    bool rememberChoice() const;

private:
    QUrl url_;
};


#endif // SEAFILE_CLIENT_UI_SSL_CONFIRM_DIALOG_H
