#include <QDialog>
#include <QUrl>
#include <QString>

#include "ui_repo-detail-dialog.h"
#include "api/server-repo.h"
#include "rpc/local-repo.h"

class QTimer;

class RepoDetailDialog : public QDialog,
                         public Ui::RepoDetailDialog
{
    Q_OBJECT
public:
    RepoDetailDialog(const ServerRepo &repo, QWidget *parent=0);

private slots:
    void updateRepoStatus();

private:
    Q_DISABLE_COPY(RepoDetailDialog)

    ServerRepo repo_;
    QTimer *refresh_timer_;
};
