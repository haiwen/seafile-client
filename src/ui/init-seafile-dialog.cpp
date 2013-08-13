#include <QtGui>
#include "init-seafile-dialog.h"

InitSeafileDialog::InitSeafileDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Choose a directory"));
}
