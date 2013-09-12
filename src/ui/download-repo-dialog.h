#include <QDialog>
#include <QUrl>
#include <QString>

#include "ui_download-repo-dialog.h"
#include "api/server-repo.h"

class DownloadRepoDialog : public QDialog,
                           public Ui::DownloadRepoDialog
{
    Q_OBJECT
public:
    DownloadRepoDialog(const ServerRepo& repo, QWidget *parent=0);

private slots:
    void syncAction();
    void chooseDirAction();

private:
    Q_DISABLE_COPY(DownloadRepoDialog);
    bool validateInputs();

    ServerRepo repo_;
};
