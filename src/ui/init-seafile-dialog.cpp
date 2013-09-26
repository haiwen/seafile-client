#include <QtGui>

#include "seafile-applet.h"
#include "configurator.h"
#include "init-seafile-dialog.h"

InitSeafileDialog::InitSeafileDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);
    setWindowTitle(tr("Choose a directory"));

    connect(mChooseDirBtn, SIGNAL(clicked()), this, SLOT(chooseDir()));
    connect(mOkBtn, SIGNAL(clicked()), this, SLOT(onOkClicked()));
}

void InitSeafileDialog::chooseDir()
{
    // TODO: on windows, set the initial path to the max volume, on linux/mac,
    // set to the home direcotry.
    QString dir = QFileDialog::getExistingDirectory(this, tr("Please choose a directory"),
                                                    QString(), // initial path
                                                    QFileDialog::ShowDirsOnly
                                                    | QFileDialog::DontResolveSymlinks);
    if (dir.isEmpty())
        return;

    mDirectory->setText(dir);
}

void InitSeafileDialog::onOkClicked()
{
    QString path;
    if (path.isEmpty()) {
        QMessageBox::warning(this, tr("Seafile"),
                             tr("Please choose the drectory to sync"),
                             QMessageBox::Ok);
        return;
    }

    QDir dir(path);
    if (dir.exists()) {
        QMessageBox::warning(this, tr("Seafile"),
                             tr("The folder %1 does not exist").arg(path),
                             QMessageBox::Ok);
        return;
    }

    dir.mkpath("Seafile/seafile-data");

    QString seafile_dir = dir.filePath("Seafile/seafile-data");
    emit seafileDirSet(seafile_dir);
}
