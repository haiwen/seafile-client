#include <QDialog>
#include <QUrl>
#include <QString>

#include "ui_repo-detail-dialog.h"
#include "api/server-repo.h"


class RepoDetailDialog : public QDialog,
                         public Ui::RepoDetailDialog
{
    Q_OBJECT
public:
    RepoDetailDialog(const ServerRepo &repo, QWidget *parent=0);
    ~RepoDetailDialog();


private:
    Q_DISABLE_COPY(RepoDetailDialog);

    ServerRepo repo_;
};
