#if defined(Q_OS_WIN)
#include <shellapi.h>
#endif

extern "C" {
#include <searpc-client.h>
#include <ccnet.h>

#include <searpc.h>
#include <seafile/seafile.h>
#include <seafile/seafile-object.h>

}

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QMessageBox>
#include <QIcon>
#include <QMainWindow>

#include "utils/utils.h"
#include "configurator.h"
#include "settings-mgr.h"
#include "ui/uninstall-helper-dialog.h"

#include "uninstall-helpers.h"

namespace {

const char *kAppletCommandsMQ = "applet.commands";

} // namespace


int delete_dir_recursively(const QString& path_in)
{
    printf ("removing folder %s\n", toCStr(path_in));
#if defined(Q_OS_WIN)
    const QString path = QDir::toNativeSeparators(QDir::cleanPath(path_in));
    if (path.length() <= 3) {
        // avoid errornous delete drives like C:/ D:/ E:/
        return -1;
    }

    int len = path.length();

    wchar_t *wpath = new wchar_t[len + 2];

    wcscpy(wpath, path.toStdWString().c_str());
    wpath[len + 1] = L'\0';

    SHFILEOPSTRUCTW fileop;
    fileop.hwnd   = NULL;       // no status display
    fileop.wFunc  = FO_DELETE;  // delete operation
    fileop.pFrom  = wpath; // source file name as double null terminated string
    fileop.pTo    = NULL;         // no destination needed
    fileop.fFlags = FOF_NOCONFIRMATION|FOF_SILENT; // do not prompt the user

    fileop.fAnyOperationsAborted = FALSE;
    fileop.lpszProgressTitle     = NULL;
    fileop.hNameMappings         = NULL;

    int ret = SHFileOperationW(&fileop);

    delete []wpath;

    if (ret == 0) {
        return 0;
    } else {
        return -1;
    }
#endif
    return 0;
}

int get_ccnet_dir(QString *ret)
{
    QString path = defaultCcnetDir();

    if (!QFileInfo(QDir(path).filePath("ccnet.conf")).exists()) {
        return -1;
    }

    *ret = path;
    return 0;
}

int get_seafile_data_dir(const QString& ccnet_dir, QString *ret)
{
    QFile seafile_ini(QDir(ccnet_dir).filePath("seafile.ini"));
    if (!seafile_ini.exists()) {
        return -1;
    }

    if (!seafile_ini.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return -1;
    }

    QTextStream input(&seafile_ini);
    input.setCodec("UTF-8");

    if (input.atEnd()) {
        return -1;
    }

    QString path = input.readLine();
    if (!QFileInfo(QDir(path).filePath("repo.db")).exists()) {
        return -1;
    }

    *ret = path;
    return 0;
}


void do_stop()
{
    CcnetClient *sync_client = ccnet_client_new();
    const QString ccnet_dir = defaultCcnetDir();
    if (ccnet_client_load_confdir(sync_client, toCStr(ccnet_dir)) <  0) {
        return;
    }

    if (ccnet_client_connect_daemon(sync_client, CCNET_CLIENT_SYNC) < 0) {
        return;
    }

    CcnetMessage *quit_message;
    quit_message = ccnet_message_new (sync_client->base.id,
                                      sync_client->base.id,
                                      kAppletCommandsMQ, "quit", 0);

    ccnet_client_send_message(sync_client, quit_message);

    ccnet_message_free(quit_message);
    g_object_unref (sync_client);
}

void do_remove_user_data()
{
    do_stop();
    set_seafile_auto_start(false);
    Configurator::removeVirtualDrive();
    SettingsManager::removeAllSettings();

    UninstallHelperDialog *dialog = new UninstallHelperDialog;

    dialog->show();
    dialog->raise();
    dialog->activateWindow();

    qApp->exec();
}
