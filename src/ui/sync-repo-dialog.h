#include <QDialog>
#include <QUrl>
#include <QString>

#include "ui_sync-repo.h"

class ServerRepo;

class SyncRepoDialog : public QDialog,
                           public Ui::SyncRepoDialog
{
    Q_OBJECT
public:
    SyncRepoDialog(ServerRepo *repo, QWidget *parent=0);
    ~SyncRepoDialog();

private slots:
    void syncAction();
    void chooseDirAction();

private:
    Q_DISABLE_COPY(SyncRepoDialog);
    bool validateInputs();

    ServerRepo *repo_;
};
