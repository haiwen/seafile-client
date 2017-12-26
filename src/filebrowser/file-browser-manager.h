#ifndef SEAFILE_CLIENT_FILE_BROWSER_MANAGER_H
#define SEAFILE_CLIENT_FILE_BROWSER_MANAGER_H
#include <QObject>
#include <QList>
#include "api/server-repo.h"
#include "account.h"

class FileBrowserDialog;

class FileBrowserManager : public QObject {
  Q_OBJECT
public:
  static FileBrowserManager* getInstance() {
    if (!instance_) {
        static FileBrowserManager instance;
        instance_ = &instance;
    }
    return instance_;
  }

  FileBrowserDialog *openOrActivateDialog(const Account &account, const ServerRepo &repo, const QString &path = "/");

  FileBrowserDialog *getDialog(const Account &account, const QString &repo_id);

public slots:
  void closeAllDialogByAccount(const Account &account);

private slots:
  void onAboutToClose();
  void closeAllDialogs();

private:
  FileBrowserManager(const FileBrowserManager*); // DELETED
  FileBrowserManager& operator=(const FileBrowserManager*); // DELETED

  FileBrowserManager();
  static FileBrowserManager *instance_;
  QList<FileBrowserDialog*> dialogs_;
};


#endif // SEAFILE_CLIENT_FILE_BROWSER_MANAGER_H
