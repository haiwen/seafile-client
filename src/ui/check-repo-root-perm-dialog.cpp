#include <QtGui>
#include <QTimer>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QProgressBar>
#include <QPushButton>

#include "filebrowser/seaf-dirent.h"
#include "filebrowser/data-mgr.h"

#include "check-repo-root-perm-dialog.h"

CheckRepoRootDirPermDialog::CheckRepoRootDirPermDialog(const Account& account, const QString& repo_id, QWidget *parent)
    : QProgressDialog(parent),
      account_(account),
      repo_id_(repo_id),
      has_write_perm_(false)
{
    // Here we use the data manager class to fetch the dir instead of doing it
    // directly, because DataManager class would make use of the shared
    // in-memory dirents cache.
    data_mgr_ = new DataManager(account_);

    setWindowModality(Qt::WindowModal);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowIcon(QIcon(":/images/seafile.png"));

    QVBoxLayout *layout_ = new QVBoxLayout;
    progress_bar_ = new QProgressBar;
    description_label_ = new QLabel;

    layout_->addWidget(description_label_);
    layout_->addWidget(progress_bar_);

    QHBoxLayout *hlayout_ = new QHBoxLayout;
    more_details_label_ = new QLabel;
    more_details_label_->setText(tr("Checking Permission"));
    QPushButton *cancel_button_ = new QPushButton(tr("Cancel"));
    QWidget *spacer = new QWidget;
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    hlayout_->addWidget(more_details_label_);
    hlayout_->addWidget(spacer);
    hlayout_->addWidget(cancel_button_);
    hlayout_->setContentsMargins(-1, 0, -1, 6);
    layout_->setContentsMargins(-1, 0, -1, 6);
    layout_->addLayout(hlayout_);

    setLayout(layout_);
    setLabel(description_label_);
    setBar(progress_bar_);
    setCancelButton(cancel_button_);

    connect(data_mgr_, SIGNAL(getDirentsSuccess(bool, const QList<SeafDirent>&)),
            this, SLOT(onGetDirentsSuccess(bool)));
    connect(data_mgr_, SIGNAL(getDirentsFailed(const ApiError&)),
            this, SLOT(onGetDirentsFailed()));

    QTimer::singleShot(0, this, SLOT(checkPerm()));
}

CheckRepoRootDirPermDialog::~CheckRepoRootDirPermDialog()
{
    delete data_mgr_;
}

void CheckRepoRootDirPermDialog::checkPerm()
{
    QList<SeafDirent> dirents;
    bool readonly;
    if (data_mgr_->getDirents(repo_id_, "/", &dirents, &readonly)) {
        onGetDirentsSuccess(readonly);
        return;
    }

    data_mgr_->getDirentsFromServer(repo_id_, "/");
}

void CheckRepoRootDirPermDialog::onGetDirentsSuccess(bool readonly)
{
    has_write_perm_ = !readonly;
    accept();
}

void CheckRepoRootDirPermDialog::onGetDirentsFailed()
{
    has_write_perm_ = false;
    accept();
}
