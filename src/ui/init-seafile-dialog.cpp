#include <QtGlobal>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QtWidgets>
#else
#include <QtGui>
#endif

#include "utils/utils.h"
#include "seafile-applet.h"
#include "configurator.h"
#include "init-seafile-dialog.h"
#include <vector>

#if defined(Q_OS_WIN32)
#include <ShlObj.h>
#include <shlwapi.h>
#endif

namespace {

#if defined(Q_OS_WIN32)
QString get_largest_drive()
{
    wchar_t drives[MAX_PATH];
    wchar_t *p, *largest_drive;
    ULARGE_INTEGER free_space;
    ULARGE_INTEGER largest_free_space;

    largest_free_space.QuadPart = 0;
    largest_drive = NULL;

    GetLogicalDriveStringsW (sizeof(drives) / sizeof(wchar_t), drives);
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
            qDebug ("failed to GetDiskFreeSpaceEx(), GLE=%lu\n",
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

QString getSystemDirectory()
{
    std::vector<wchar_t> path;
    path.resize(MAX_PATH);

    UINT len = ::GetSystemDirectoryW(&path[0], MAX_PATH);
    path.resize(len);
    if (len > MAX_PATH) {
        len = ::GetSystemDirectoryW(&path[0], len);
    }
    return QString::fromWCharArray(&path[0], (int)len);
}

inline bool hasSameDrive(const QString& path_a, const QString &path_b)
{
    // to detect drive letter of a file, use QFileInfo::absoluteFilePath
    //                                   (D:\)
    // On Windows this will always begin 'D:/' where D is a drive letter, except
    // for network shares that are not mapped to a drive letter
    // same driver letter?
    if (path_a.length() < 3 || path_b.length() < 3)
        return false;
    if (path_a[0] != path_b[0])
        return false;
    if (path_a[1] != ':' || (path_a[2] != '/' && path_a[2] != '\\') ||
        path_b[1] != ':' || (path_b[2] != '/' && path_b[2] != '\\'))
        return false;
    return true;
}

#endif


} // namespace

InitSeafileDialog::InitSeafileDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);

    setWindowTitle(tr("%1 Initialization").arg(getBrand()));
    setWindowIcon(QIcon(":/images/seafile.png"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    connect(mChooseDirBtn, SIGNAL(clicked()), this, SLOT(chooseDir()));
    connect(mOkBtn, SIGNAL(clicked()), this, SLOT(onOkClicked()));
    connect(mCancelBtn, SIGNAL(clicked()), this, SLOT(onCancelClicked()));

    mLogo->setPixmap(QPixmap(":/images/seafile-32.png"));
    mDirectory->setText(getInitialPath());

    const QRect screen = QApplication::desktop()->screenGeometry();
    move(screen.center() - this->rect().center());
}

QString InitSeafileDialog::getInitialPath()
{
#if defined(Q_OS_WIN32)
    QString largest_drive = get_largest_drive();
    QString home_path = QDir::home().absolutePath();
    QString sys_path = getSystemDirectory();
    if (hasSameDrive(largest_drive, sys_path) ||
        hasSameDrive(largest_drive, home_path))
        return home_path;

    // work around with UTF-16 bug?
    return QString::fromUtf8(largest_drive.toUtf8());
#else
    return QDir::home().path();
#endif
}

void InitSeafileDialog::chooseDir()
{
    QString initial_path;

    // On windows, set the initial path to the max volume, on linux/mac, set
    // to the home direcotry.
    QString dir = QFileDialog::getExistingDirectory(this, tr("Please choose a directory"),
                                                    getInitialPath(),
                                                    QFileDialog::ShowDirsOnly
                                                    | QFileDialog::DontResolveSymlinks);
    if (dir.isEmpty())
        return;

    mDirectory->setText(dir);
}

void InitSeafileDialog::onOkClicked()
{
    QString path = mDirectory->text();
    if (path.isEmpty()) {
        QMessageBox::warning(this, getBrand(),
                             tr("Please choose a directory"),
                             QMessageBox::Ok);
        return;
    }

    QDir dir(path);
    if (!dir.exists()) {
        QMessageBox::warning(this, getBrand(),
                             tr("The folder %1 does not exist").arg(path),
                             QMessageBox::Ok);
        return;
    }

#if defined(Q_OS_WIN32)
    dir.mkpath("Seafile/seafile-data");
    QString seafile_dir = dir.filePath("Seafile/seafile-data");
#else
    dir.mkpath("Seafile/.seafile-data");
    QString seafile_dir = dir.filePath("Seafile/.seafile-data");
#endif

    emit seafileDirSet(seafile_dir);

    accept();
}

void InitSeafileDialog::onCancelClicked()
{
    QString question = tr("Initialization is not finished. Really quit?");
    if (QMessageBox::question(this, getBrand(),
                              question,
                              QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::No) == QMessageBox::Yes) {
        reject();
    }
}

void InitSeafileDialog::closeEvent(QCloseEvent *event)
{
    QString question = tr("Initialization is not finished. Really quit?");
    if (QMessageBox::question(this, getBrand(),
                              question,
                              QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::No) == QMessageBox::Yes) {
        QDialog::closeEvent(event);
        return;
    }
    event->ignore();
}
