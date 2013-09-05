#include <QDialog>
#include <QUrl>
#include <QString>

#include "ui_sync-repo.h"
#include "api/server-repo.h"

class SyncRepoDialog : public QDialog,
                           public Ui::SyncRepoDialog
{
    Q_OBJECT
public:
    SyncRepoDialog(const ServerRepo& repo, QWidget *parent=0);

private slots:
    void syncAction();
    void chooseDirAction();

private:
    Q_DISABLE_COPY(SyncRepoDialog);
    bool validateInputs();

    ServerRepo repo_;
};
