#ifndef SEAFILE_CLIENT_UNINSTALL_HELPER_WINDOW_H
#define SEAFILE_CLIENT_UNINSTALL_HELPER_WINDOW_H

#include <QThread>
#include <QDialog>
#include "ui_uninstall-helper-dialog.h"


class UninstallHelperDialog : public QDialog,
                              public Ui::UninstallHelperDialog
{
    Q_OBJECT
public:
    UninstallHelperDialog(QWidget *parent=0);

private slots:
    void onYesClicked();
    void doExit();

private:
    Q_DISABLE_COPY(UninstallHelperDialog)

    bool loadQss(const QString& path);

    QString style_;
};

class RemoveSeafileDataThread : public QThread
{
    Q_OBJECT
public:
    void run();
};


#endif // SEAFILE_CLIENT_UNINSTALL_HELPER_WINDOW_H
