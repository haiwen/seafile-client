#include "file-browser-manager.h"

#include <QApplication>
#include <QDesktopWidget>

#include "file-browser-dialog.h"

namespace {
}

FileBrowserManager *FileBrowserManager::instance_ = NULL;


FileBrowserDialog *FileBrowserManager::openOrActivateDialog(const Account &account, const ServerRepo &repo)
{
    FileBrowserDialog *dialog = getDialog(account, repo.id);
    if (dialog == NULL) {
        dialog = new FileBrowserDialog(account, repo);
        QRect screen = QApplication::desktop()->screenGeometry();
        dialog->setAttribute(Qt::WA_DeleteOnClose, true);
        dialog->show();
        dialog->move(screen.center() - dialog->rect().center());
        dialogs_.push_back(dialog);
        connect(dialog, SIGNAL(aboutToClose()), this, SLOT(onAboutToClose()));
    }
    dialog->raise();
    dialog->activateWindow();
    return dialog;
}

FileBrowserDialog *FileBrowserManager::getDialog(const Account &account, const QString &repo_id)
{
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
    Q_FOREACH(FileBrowserDialog *dialog, dialogs_)
    {
        if (dialog->account_ == account)
            dialog->deleteLater();
    }
}

void FileBrowserManager::onAboutToClose()
{
    FileBrowserDialog *dialog = qobject_cast<FileBrowserDialog*>(sender());
    if (!dialog)
      return;
    dialogs_.removeOne(dialog);
}
