#include <QtGui>
#include <QMovie>

#include "list-account-repos-dialog.h"
#include "api/server-repo.h"
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
    mScrollArea->setWidget(list_widget_);
    mScrollArea->setVisible(false);

    sendRequest();
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
