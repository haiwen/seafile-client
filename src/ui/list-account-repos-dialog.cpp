#include <QtGui>
#include <QMovie>

#include "list-account-repos-dialog.h"
#include "api/requests.h"

ListAccountReposDialog::ListAccountReposDialog(const Account& account,
                                               QWidget *parent)
    : QDialog(parent),
      account_(account)
{
    setupUi(this);

    setWindowTitle(tr("Account Libraries - %1").arg(account.username));

    mErrorText->setText(tr("failed to get libraries"));
    mErrorText->setVisible(false);

    mLoadingMovie->setMovie(new QMovie(":/images/loading.gif"));

    list_widget_ = new QListWidget;
    list_widget_->setSelectionMode(QAbstractItemView::MultiSelection);
    mScrollArea->setWidget(list_widget_);
    mScrollArea->setVisible(false);

    sendRequest();

    connect(downloadRepoBtn, SIGNAL(clicked()), this, SLOT(downloadRepos()));
}

void ListAccountReposDialog::downloadRepos()
{
    QList<QListWidgetItem *> selected = list_widget_->selectedItems();

    qDebug("download %d repos ...\n", selected.count());

    QListIterator<QListWidgetItem *> itr(selected);
    while (itr.hasNext()) {
        int row = list_widget_->row(itr.next());
        ServerRepo &repo = repos_[row];
        qDebug() << "download repo " << repo.id << repo.name ;
    }
}

void ListAccountReposDialog::sendRequest()
{
    request_ = new ListReposRequest(account_);

    connect(request_, SIGNAL(success(const std::vector<ServerRepo>&)),
            this, SLOT(onRequestSuccess(const std::vector<ServerRepo>&)));

    connect(request_, SIGNAL(failed(int)), this, SLOT(onRequestFailed(int)));

    request_->send();
}

void ListAccountReposDialog::onRequestSuccess(const std::vector<ServerRepo>& repos)
{
    qDebug("account repos dialog: %d repos\n", (int)repos.size());
    QIcon repo_icon(":/images/repo.png");
    repos_ = repos;
    for (std::vector<ServerRepo>::const_iterator iter = repos.begin();
         iter != repos.end(); iter++) {
        const ServerRepo& repo = *iter;
        QListWidgetItem *item = new QListWidgetItem(repo_icon, repo.name);
        list_widget_->addItem(item);
    }
    mScrollArea->setVisible(true);
}

void ListAccountReposDialog::onRequestFailed(int code)
{
    mScrollArea->setVisible(false);
    mLoadingMovie->setVisible(false);

    mErrorText->setVisible(true);
}
