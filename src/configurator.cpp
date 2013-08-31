#include <QDir>
#include <QFile>
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
    if (dialog.exec() != QDialog::Accepted) {
        seafApplet->exit(1);
    }
}

void Configurator::validateExistingConfig()
{
    QFile ccnet_conf(QDir(ccnet_dir_).filePath("ccnet.conf"));
    if (!ccnet_conf.exists()) {
        // TODO: init config here instead of exit
        seafApplet->errorAndExit(tr("%1 not found").arg(ccnet_conf.fileName()));
    }

    if (readSeafileIni(&seafile_dir_) < 0) {
        // TODO: init config here instead of exit
        seafApplet->errorAndExit(tr("seafile.ini not found"));
    }

    if (!QDir(seafile_dir_).exists()) {
        // TODO: init config here instead of exit
        seafApplet->errorAndExit(tr("%1 does not exist").arg(seafile_dir_));
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
