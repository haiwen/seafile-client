#ifndef SEAFILE_CLIENT_WELCOME_DIALOG_H
#define SEAFILE_CLIENT_WELCOME_DIALOG_H

#include <QDialog>
#include "ui_welcome-dialog.h"

class WelcomeDialog : public QDialog,
                      public Ui::WelcomeDialog
{
    Q_OBJECT
public:
    WelcomeDialog(QWidget *parent=0);

private:
    Q_DISABLE_COPY(WelcomeDialog);
};


#endif // SEAFILE_CLIENT_WELCOME_DIALOG_H
