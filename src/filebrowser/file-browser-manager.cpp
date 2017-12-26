#include "file-browser-manager.h"

#include <QApplication>
#include <QDesktopWidget>

#include "seafile-applet.h"
#include "account-mgr.h"

#include "file-browser-dialog.h"

namespace {
}

FileBrowserManager *FileBrowserManager::instance_ = NULL;

FileBrowserManager::FileBrowserManager() : QObject()
{
    // We use the accountAboutToRelogin signal instead of the
    // accountRequireRelogin signal because other components are already
    // connected to the the latter and may prompt a re-login dialog which would
    // delay the call to our slots.
    connect(seafApplet->accountManager(),
            SIGNAL(accountAboutToRelogin(const Account&)),
            this,
            SLOT(closeAllDialogByAccount(const Account&)));

    connect(seafApplet->accountManager(), SIGNAL(beforeAccountSwitched()),
            this, SLOT(closeAllDialogs()));
}


FileBrowserDialog *FileBrowserManager::openOrActivateDialog(const Account &account, const ServerRepo &repo, const QString &path)
{
    FileBrowserDialog *dialog = getDialog(account, repo.id);
    QString fixed_path = path;
    if (!fixed_path.startsWith("/")) {
        fixed_path = "/" + fixed_path;
    }
    if (!fixed_path.endsWith("/")) {
        fixed_path += "/";
    }
    if (dialog == NULL) {
        dialog = new FileBrowserDialog(account, repo, fixed_path);
        QRect screen = QApplication::desktop()->screenGeometry();
        dialog->setAttribute(Qt::WA_DeleteOnClose, true);
        dialog->show();
        dialog->move(screen.center() - dialog->rect().center());
        dialogs_.push_back(dialog);
        connect(dialog, SIGNAL(aboutToClose()), this, SLOT(onAboutToClose()));
    } else if (!path.isEmpty()) {
        dialog->enterPath(fixed_path);
    }
    dialog->raise();
    dialog->activateWindow();
    return dialog;
}

FileBrowserDialog *FileBrowserManager::getDialog(const Account &account, const QString &repo_id)
{
    // printf ("Get dialog: current %u CFB\n", dialogs_.size());
    // search and find if dialog registered
    for (int i = 0; i < dialogs_.size() ; i++)
        if (dialogs_[i]->account_ == account &&
            dialogs_[i]->repo_.id == repo_id) {
            return dialogs_[i];
        }
    // not found
    return NULL;
}

void FileBrowserManager::closeAllDialogByAccount(const Account& account)
{
    // printf ("closeAllDialogByAccount is called\n");

    // Close all dialogs for the given account, e.g. when logging out or
    // deleteing an account.

    // Note: DO NOT remove close the dialog while iterating the dialogs list,
    // because close the dialog would make it removed from the list (through the
    // onAboutToClose signal). Instead we first collect the matched dialogs into
    // a temporary list, then close them one by one.
    QList<FileBrowserDialog *> dialogs_for_account;
    foreach (FileBrowserDialog *dialog, dialogs_)
    {
        if (dialog->account_ == account) {
            dialogs_for_account.push_back(dialog);
        }
    }

    foreach (FileBrowserDialog *dialog, dialogs_for_account)
    {
        dialog->close();
        // printf ("closed one CFB\n");
    }
}

void FileBrowserManager::onAboutToClose()
{
    // printf ("got onAboutToClose\n");
    FileBrowserDialog *dialog = qobject_cast<FileBrowserDialog*>(sender());
    if (!dialog)
      return;
    dialogs_.removeOne(dialog);
}

void FileBrowserManager::closeAllDialogs()
{
    QList<FileBrowserDialog *> dialogs_for_account;
    foreach (FileBrowserDialog *dialog, dialogs_)
    {
        dialogs_for_account.push_back(dialog);
    }

    foreach (FileBrowserDialog *dialog, dialogs_for_account)
    {
        dialog->close();
    }
}
