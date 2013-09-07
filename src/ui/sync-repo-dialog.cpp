#include <QtGui>

#include "account-mgr.h"
#include "seafile-applet.h"
#include "configurator.h"
#include "api/requests.h"
#include "api/server-repo.h"
#include "sync-repo-dialog.h"

SyncRepoDialog::SyncRepoDialog(const ServerRepo& repo, QWidget *parent)
    : QDialog(parent),
      repo_(repo)
{
    setupUi(this);
    setWindowTitle(tr("Sync repo \"%1\"").arg(repo_.name));

    mDirectory->setText(seafApplet->configurator()->worktreeDir());
    if (repo_.encrypted) {
        mPassword->setVisible(true);
        mLabelPassword->setVisible(true);
    } else {
        mLabelPassword->setVisible(false);
        mPassword->setVisible(false);
    }

    connect(mChooseDirBtn, SIGNAL(clicked()), this, SLOT(chooseDirAction()));
    connect(mSyncBtn, SIGNAL(clicked()), this, SLOT(syncAction()));
}

void SyncRepoDialog::chooseDirAction()
{
    const QString &wt = seafApplet->configurator()->worktreeDir();
    QString dir = QFileDialog::getExistingDirectory(this, tr("Please choose a directory"),
                                                        wt.toUtf8().data(),
                                                        QFileDialog::ShowDirsOnly
                                                        | QFileDialog::DontResolveSymlinks);

    qDebug() << "Choose directory:" << dir;
    if (dir.isEmpty())
        return;
    mDirectory->setText(dir);
}

void SyncRepoDialog::syncAction()
{
    if (!validateInputs()) {
        return;
    }

    done(QDialog::Accepted);
}

bool SyncRepoDialog::validateInputs()
{

    if (mDirectory->text().isEmpty()) {
        QMessageBox::warning(this, tr("Seafile"),
                             tr("Please choose the drectory to sync"),
                             QMessageBox::Ok);
        return false;
    }
    QDir dir(mDirectory->text());
    if (!dir.exists()) {
        QMessageBox::warning(this, tr("Seafile"),
                             tr("Directory donot exist"),
                             QMessageBox::Ok);
        return false;
    }
    if (repo_.encrypted) {
        if (mPassword->text().isEmpty()) {
            QMessageBox::warning(this, tr("Seafile"),
                                 tr("Please enter the password"),
                                 QMessageBox::Ok);
            return false;
        }
        // repo_.passwd = mPassword->text();
    }
    // repo_.localdir = mDirectory->text();
    // repo_.download = (mRadioDownload->isChecked()) ? true : false;
    return true;
}
