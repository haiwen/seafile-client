#ifndef SEAFILE_CLIENT_ABOUT_DIALOG_H
#define SEAFILE_CLIENT_ABOUT_DIALOG_H

#include <QDialog>
#include "ui_about-dialog.h"

class AutoUpdateService;

class AboutDialog : public QDialog,
                    public Ui::AboutDialog
{
    Q_OBJECT
public:
    AboutDialog(QWidget *parent=0);

#ifdef HAVE_SPARKLE_SUPPORT
private slots:
    void checkUpdate();
#endif

private:
    Q_DISABLE_COPY(AboutDialog)

    QString version_text_;
};

#endif // SEAFILE_CLIENT_ABOUT_DIALOG_H
