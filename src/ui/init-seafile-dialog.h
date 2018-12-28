#ifndef SEAFILE_CLIENT_INIT_SEAFILE_DIALOG_H
#define SEAFILE_CLIENT_INIT_SEAFILE_DIALOG_H

#if !defined(Q_OS_WIN32)
#include <sys/stat.h>
#endif

#include <QDialog>
#include "ui_init-seafile-dialog.h"

class QCloseEvent;

class InitSeafileDialog : public QDialog,
                          public Ui::InitSeafileDialog
{
    Q_OBJECT

public:
    InitSeafileDialog(QWidget *parent=0);
    void closeEvent(QCloseEvent *event);

private slots:
    void onOkClicked();
    void onCancelClicked();
    void chooseDir();

signals:
    void seafileDirSet(const QString&);

private:
    QString getInitialPath();
};

#endif  // SEAFILE_CLIENT_INIT_SEAFILE_DIALOG_H
