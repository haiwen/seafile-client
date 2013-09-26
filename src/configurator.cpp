#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDebug>

#include "seafile-applet.h"
#include "ccnet-init.h"
#include "ui/init-seafile-dialog.h"
#include "configurator.h"


namespace {

#if defined(Q_WS_WIN)
const char *kCcnetConfDir = "ccnet";
#else
const char *kCcnetConfDir = ".ccnet";
#endif

QString defaultCcnetDir() {
    return QDir::home().filePath(kCcnetConfDir);
}

} // namespace


Configurator::Configurator()
    : ccnet_dir_(defaultCcnetDir())
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
    initCcnet();
    initSeafile();
}

void Configurator::initCcnet()
{
    QString path = QDir::toNativeSeparators(ccnet_dir_);
    if (create_ccnet_config(path.toUtf8().data()) < 0) {
        seafApplet->errorAndExit(tr("Error when creating ccnet configuration"));
    }
}

void Configurator::initSeafile()
{
    InitSeafileDialog dialog;
    connect(&dialog, SIGNAL(seafileDirSet(const QString&)),
            this, SLOT(onSeafileDirSet(const QString&)));

    if (dialog.exec() != QDialog::Accepted) {
        seafApplet->exit(1);
    }
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
#if defined(Q_WS_WIN)
    std::wstring seafdir = seafile_dir_.toStdWString();

    // Make seafile-data folder hidden
    SetFileAttributesW (seafdir.c_str(),
                        FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);

    // Set seafdir folder icon.
    SetFileAttributesW (seafdir.c_str(), FILE_ATTRIBUTE_SYSTEM);
    QString desktop_ini_path = QDir(seafile_dir_).filePath("Desktop.ini");
    QFile desktop_ini(desktop_ini_path);

    if (!desktop_ini.open(QIODevice::WriteOnly |  QIODevice::Text)) {
        return;
    }

    QString icon_path = QDir(QCoreApplication::applicationDirPath()).filePath("seafdir.ico");

    QTextStream out(&desktop_ini);
    out << "[.ShellClassInfo]\n";
    out << QString("IconFile=%1\n").arg(icon_path);
    out << "IconIndex=0\n";

    // Make the "Desktop.ini" file hidden.
    SetFileAttributesW (desktop_ini_path.toStdWString().c_str(),
                        FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
#endif
}

void Configurator::validateExistingConfig()
{
    QFile ccnet_conf(QDir(ccnet_dir_).filePath("ccnet.conf"));
    if (!ccnet_conf.exists()) {
        initConfig();
        return;
    }

    if (readSeafileIni(&seafile_dir_) < 0 || !QDir(seafile_dir_).exists()) {
        initSeafile();
        return;
    }

    QFileInfo finfo(seafile_dir_);
    worktree_ = finfo.dir().path();
}

int Configurator::readSeafileIni(QString *content)
{
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
