#include "file-browser-manager.h"

#include "seafile-applet.h"
#include "ui/main-window.h"
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
        dialog = new FileBrowserDialog(account, repo, seafApplet->mainWindow());
        QRect screen = QApplication::desktop()->screenGeometry();
        dialog->setAttribute(Qt::WA_DeleteOnClose, true);
        dialog->show();
        dialog->move(screen.center() - dialog->rect().center());
        dialogs_.push_back(dialog);
    }
    dialog->raise();
    return dialog;
}

FileBrowserDialog *FileBrowserManager::getDialog(const Account &account, const QString &repo_id)
{
    // remove all QWeakPointer which is NULL alreadly;
    for (int i = dialogs_.size() - 1; i >= 0; i--)
        if (dialogs_[i].isNull())
          dialogs_.removeAt(i);

    // search and find if dialog registered
    for (int i = 0; i < dialogs_.size() ; i++)
        if (dialogs_[i].data()->account_ == account &&
            dialogs_[i].data()->repo_.id == repo_id) {
            return dialogs_[i].data();
        }
    // not found
    return NULL;
}

