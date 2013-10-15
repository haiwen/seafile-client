#include <QtGui>

#include "welcome-dialog.h"

WelcomeDialog::WelcomeDialog(QWidget *parent) : QDialog(parent)
{
    setupUi(this);
    setWindowTitle(tr("Seafile Initialzation"));
    setWindowIcon(QIcon(":/images/seafile.png"));
}
