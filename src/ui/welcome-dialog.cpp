#include <QtGui>
#include <QPixmap>

#include "welcome-dialog.h"
#include "seafile-applet.h"

WelcomeDialog::WelcomeDialog(QWidget *parent) : QDialog(parent)
{
    setupUi(this);
    setWindowTitle(tr("%1 Initialzation").arg(SEAFILE_CLIENT_BRAND));
    setWindowIcon(QIcon(":/images/seafile.png"));

    mHelpImage->setPixmap(QPixmap(":/images/seafile-download-library.png"));
}
