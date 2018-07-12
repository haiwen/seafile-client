#include <glib.h>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDebug>
#include <QList>
#include <QMessageBox>
#include <QSettings>

#include "utils/utils.h"
#include "utils/file-utils.h"
#include "seafile-applet.h"
#include "ui/init-seafile-dialog.h"

#if defined(Q_OS_WIN32)
#include "utils/registry.h"
#include <shlobj.h>
#include <shlwapi.h>
#endif

#include "configurator.h"


namespace {

#if defined(Q_OS_WIN32)
const char *kVirtualDriveGUID = "F817C393-A76E-435E-B6B1-485844BC9C2E";
const char *kMyComputerNamespacePath =
    "Software\\Microsoft\\Windows\\CurrentVersion"
    "\\Explorer\\MyComputer\\Namespace";
#endif

const char* const kPreconfigureDirectory = "PreconfigureDirectory";

inline QString getPreconfigureDirectory()
{
    return expandVars(expandUser(seafApplet->readPreconfigureEntry(kPreconfigureDirectory).toString()));
}

} // namespace


Configurator::Configurator()
    : ccnet_dir_(defaultCcnetDir()),
      first_use_(false)
{
}

void Configurator::checkInit()
{
    if (needInitConfig()) {
        // first time use
        initConfig();
    } else {
        validateExistingConfig();
    }
}

bool Configurator::needInitConfig()
{
    if (QDir(ccnet_dir_).exists()) {
        return false;
    }

    return true;
}

void Configurator::initConfig()
{
    QString path = QDir::toNativeSeparators(ccnet_dir_);
    QDir ccnet_dir(ccnet_dir_);
    if (!ccnet_dir.mkpath(".")) {
        seafApplet->errorAndExit(tr("Error when creating ccnet configuration"));
        return;
    }

    first_use_ = true;
    initSeafile();
}

void Configurator::initSeafile()
{
    QString preconfigure_dir = getPreconfigureDirectory();
    if (!preconfigure_dir.isEmpty()) {
        QDir dir(preconfigure_dir);
        if (!dir.mkpath(".") ||
            !dir.mkpath("Seafile/.seafile-data")) {
            qWarning("[Configurator] unable to create preconfigure directory \"%s\"",
                     preconfigure_dir.toUtf8().data());
            qWarning("[Configurator] exiting from an unrecovable error.");
            seafApplet->warningBox(
                tr("Unable to create preconfigure directory \"%1\"").arg(
                    preconfigure_dir.toUtf8().data()));
            exit(1);
            return;
        }
        first_use_ = true;

        QString seafile_dir = dir.absoluteFilePath("Seafile/.seafile-data");
        onSeafileDirSet(seafile_dir);
        return;
    }
    InitSeafileDialog dialog;
    connect(&dialog, SIGNAL(seafileDirSet(const QString&)),
            this, SLOT(onSeafileDirSet(const QString&)));

    if (dialog.exec() != QDialog::Accepted) {
        exit(1);
        return;
    }

    first_use_ = true;
}

void Configurator::onSeafileDirSet(const QString& path)
{
    // Write seafile dir to <ccnet dir>/seafile.ini
    QFile seafile_ini(QDir(ccnet_dir_).filePath("seafile.ini"));

    if (!seafile_ini.open(QIODevice::WriteOnly)) {
        return;
    }

    seafile_ini.write(path.toUtf8().data());

    seafile_dir_ = path;

    QDir d(path);

    d.cdUp();
    worktree_ = d.absolutePath();

    setSeafileDirAttributes();
}

void Configurator::setSeafileDirAttributes()
{
#if defined(Q_OS_WIN32)
    std::wstring seafdir = seafile_dir_.toStdWString();

    // Make seafile-data folder hidden
    SetFileAttributesW (seafdir.c_str(),
                        FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);

    // Set seafdir folder icon.
    SetFileAttributesW (worktree_.toStdWString().c_str(), FILE_ATTRIBUTE_SYSTEM);
    QString desktop_ini_path = QDir(worktree_).filePath("Desktop.ini");
    QFile desktop_ini(desktop_ini_path);

    if (!desktop_ini.open(QIODevice::WriteOnly |  QIODevice::Text)) {
        return;
    }

    // QString icon_path = QDir(QCoreApplication::applicationDirPath()).filePath("seafdir.ico");

    // QTextStream out(&desktop_ini);
    // out << "[.ShellClassInfo]\n";
    // out << QString("IconFile=%1\n").arg(icon_path);
    // out << "IconIndex=0\n";

    // // Make the "Desktop.ini" file hidden.
    // SetFileAttributesW (desktop_ini_path.toStdWString().c_str(),
    //                     FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
#endif
}

void Configurator::validateExistingConfig()
{
    QFile seafile_ini(QDir(ccnet_dir_).filePath("seafile.ini"));
    if (!seafile_ini.exists()) {
        initConfig();
        return;
    }

    if (readSeafileIni(&seafile_dir_) < 0 || !QDir(seafile_dir_).exists()) {
        initSeafile();
        return;
    }

    QDir d(seafile_dir_);
#if !defined(Q_OS_WIN32)
    QString old_client_wt = d.filePath("../seafile/");
    if (QFile(old_client_wt).exists()) {
        // old client
        worktree_ = QFileInfo(old_client_wt).absoluteFilePath();
        return;
    }
#endif

    d.cdUp();
    worktree_ = d.absolutePath();
}

int Configurator::readSeafileIni(QString *content)
{
    // First try SEAFILE_DATA_DIR
    const char *env = g_getenv("SEAFILE_DATA_DIR");
    if (env) {
        *content = QString::fromUtf8(env);
        return 0;
    }

    QFile seafile_ini(QDir(ccnet_dir_).filePath("seafile.ini"));
    if (!seafile_ini.exists()) {
        return -1;
    }

    if (!seafile_ini.open(QIODevice::ReadOnly | QIODevice::Text)) {
        seafApplet->errorAndExit(tr("failed to read %1").arg(seafile_ini.fileName()));
    }

    QTextStream input(&seafile_ini);
    input.setCodec("UTF-8");

    if (input.atEnd()) {
        return -1;
    }

    *content = input.readLine();

    return 0;
}

void Configurator::installCustomUrlHandler()
{
#if defined(Q_OS_WIN32)
    QList<RegElement> list;
    HKEY root = HKEY_CURRENT_USER;

    QString exe = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());

    QString cmd = QString("\"%1\" -f ").arg(exe) + " \"%1\"";

    QString classes_seafile = "Software\\Classes\\seafile";

    list.append(RegElement(root, classes_seafile,
                           "", "URL:seafile Protocol"));

    list.append(RegElement(root, classes_seafile,
                           "URL Protocol", ""));

    list.append(RegElement(root, classes_seafile + "\\shell",
                           "", ""));

    list.append(RegElement(root, classes_seafile + "\\shell\\open",
                           "", ""));

    list.append(RegElement(root, classes_seafile + "\\shell\\open\\command",
                           "", cmd));
    for (int i = 0; i < list.size(); i++) {
        RegElement& reg = list[i];
        reg.add();
    }
#endif
}
