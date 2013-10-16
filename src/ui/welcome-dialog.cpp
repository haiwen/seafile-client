#include <QtGui>
#include <QPixmap>

#include "welcome-dialog.h"

WelcomeDialog::WelcomeDialog(QWidget *parent) : QDialog(parent)
{
    setupUi(this);
    setWindowTitle(tr("Seafile Initialzation"));
    setWindowIcon(QIcon(":/images/seafile.png"));

    mHelpImage->setPixmap(QPixmap(":/images/seafile-download-library.png"));
}
