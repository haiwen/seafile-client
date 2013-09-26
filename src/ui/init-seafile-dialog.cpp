#include <QtGui>

#include "seafile-applet.h"
#include "configurator.h"
#include "init-seafile-dialog.h"

namespace {

#if defined(Q_WS_WIN)
static QString
get_largest_drive()
{
    wchar_t drives[SEAF_PATH_MAX];
    wchar_t *p, *largest_drive;
    ULARGE_INTEGER free_space;
    ULARGE_INTEGER largest_free_space;

    largest_free_space.QuadPart = 0;
    largest_drive = NULL;

    GetLogicalDriveStringsW (sizeof(drives), drives);
    for (p = drives; *p != L'\0'; p += wcslen(p) + 1) {
        /* Skip floppy disk, network drive, etc */
        if (GetDriveTypeW(p) != DRIVE_FIXED)
            continue;

        if (GetDiskFreeSpaceExW (p, &free_space, NULL, NULL)) {
            if (free_space.QuadPart > largest_free_space.QuadPart) {
                largest_free_space.QuadPart = free_space.QuadPart;
                if (largest_drive != NULL) {
                    free (largest_drive);
                }
                largest_drive = wcsdup(p);
            }

        } else {
            applet_warning ("failed to GetDiskFreeSpaceEx(), GLE=%lu\n",
                            GetLastError());
        }
    }

    QString ret;
    if (largest_drive) {
        ret = QString::fromStdWString(largest_drive);
        free(largest_drive);
    }

    return ret;
}
#endif


} // namespace

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
    QString initial_path;

    // On windows, set the initial path to the max volume, on linux/mac, set
    // to the home direcotry.
#if defined(Q_WS_WIN)
    initial_path = get_largest_drive();
#else
    initial_path = QDir::home().path();
#endif
    QString dir = QFileDialog::getExistingDirectory(this, tr("Please choose a directory"),
                                                    initial_path,
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

