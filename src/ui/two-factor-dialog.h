#ifndef TWOFACTORDIALOG_H
#define TWOFACTORDIALOG_H

#include <QDialog>
#include "ui_two-factor-dialog.h"
class TwoFactorDialog : public QDialog,
                        public Ui::TwoFactorDialog
{
    Q_OBJECT

public:
    TwoFactorDialog(QWidget *parent = 0);
    QString getText();
    bool rememberDeviceChecked();

private slots:
    void doSubmit();
};

#endif // TWOFACTORDIALOG_H
