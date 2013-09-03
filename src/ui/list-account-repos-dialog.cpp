#include <QtGui>
#include <QMovie>

#include "seafile-applet.h"
#include "configurator.h"
#include "api/requests.h"
#include "rpc/rpc-request.h"
#include "create-repo-dialog.h"
#include "sync-repo-dialog.h"
#include "list-account-repos-dialog.h"
    
ListAccountReposDialog::ListAccountReposDialog(const Account& account,
                                               QWidget *parent)
    : QDialog(parent),
      request_(NULL),
      account_(account)
{
    setupUi(this);

    setWindowTitle(tr("Account Libraries - %1").arg(account.username));

    mErrorText->setText(tr("failed to get libraries"));
    mErrorText->setVisible(false);

    mLoadingMovie->setMovie(new QMovie(":/images/loading.gif"));

    list_widget_ = new QListWidget;
    //list_widget_->setSelectionMode(QAbstractItemView::MultiSelection);
    mScrollArea->setWidget(list_widget_);
    mScrollArea->setVisible(false);

    sendListReposRequest();

    connect(syncRepoBtn, SIGNAL(clicked()), this, SLOT(syncRepoAction()));
    connect(createRepoBtn, SIGNAL(clicked()), this, SLOT(createRepoAction()));
}

ListAccountReposDialog::~ListAccountReposDialog()
{
    if (request_)
        delete request_;
    delete list_widget_;
}

void ListAccountReposDialog::syncRepoAction()
{
    QList<QListWidgetItem *> selected = list_widget_->selectedItems();

    qDebug("sync %d repos ...\n", selected.count());
    QListIterator<QListWidgetItem *> itr(selected);
    while (itr.hasNext()) {
        int row = list_widget_->row(itr.next());
        ServerRepo &repo = repos_[row];
        SyncRepoDialog dialog(&repo, this);
        if (dialog.exec() == QDialog::Accepted) {
            qDebug() << "sync repo " << repo.id << repo.name ;
            repo.req = new DownloadRepoRequest(account_, &repo);
            connect(repo.req, SIGNAL(success(const QMap<QString, QString> &, ServerRepo *)),
                    this, SLOT(onDownloadApiSuccess(const QMap<QString, QString> &, ServerRepo *)));
            connect(repo.req, SIGNAL(fail(int, ServerRepo *)), this, SLOT(onDownloadApiFailed(int, ServerRepo *)));
            repo.req->send();
        }
    }
}

void ListAccountReposDialog::createRepoAction()
{
    CreateRepoDialog dialog(account_, this);
    if (dialog.exec() == QDialog::Accepted) {
        sendListReposRequest();
    }
}

void ListAccountReposDialog::sendListReposRequest()
{
    if (!request_)
        delete request_;

    request_ = new ListReposRequest(account_);
    connect(request_, SIGNAL(success(const std::vector<ServerRepo>&)),
            this, SLOT(onListApiSuccess(const std::vector<ServerRepo>&)));
    connect(request_, SIGNAL(failed(int)), this, SLOT(onListApiFailed(int)));
    request_->send();
}

void ListAccountReposDialog::onListApiSuccess(const std::vector<ServerRepo>& repos)
{
    qDebug("account repos dialog: %d repos\n", (int)repos.size());
    QIcon repo_icon(":/images/repo.png");
    repos_ = repos;

    while (list_widget_->count() > 0) {
        if (list_widget_->item(0) != NULL) {
            QListWidgetItem *item = list_widget_->item(0);
            delete item;
        }
    }
    for (std::vector<ServerRepo>::const_iterator iter = repos.begin();
         iter != repos.end(); iter++) {
        const ServerRepo& repo = *iter;
        QListWidgetItem *item = new QListWidgetItem(repo_icon, repo.name);
        list_widget_->addItem(item);
    }
    mScrollArea->setVisible(true);
}

void ListAccountReposDialog::onListApiFailed(int code)
{
    mScrollArea->setVisible(false);
    mLoadingMovie->setVisible(false);

    mErrorText->setVisible(true);
}

void ListAccountReposDialog::onDownloadApiSuccess(const QMap<QString, QString> &dict, ServerRepo *repo)
{
    qDebug() << "onDownloadApiSuccess repo " << repo->id \
             << "localdir=" << repo->localdir            \
             << ", download:" << repo->download;

    SeafileRpcRequest *req = new SeafileRpcRequest();
    if (repo->download) {
        connect(req, SIGNAL(downloadRepoSignal(QString &, bool)),
                this, SLOT(downloadRepoRequestFinished(QString &, bool)));

        req->downloadRepo(dict["repo_id"],
                          dict["relay_id"],
                          dict["repo_name"],
                          repo->localdir,
                          dict["token"],
                          repo->passwd,
                          dict["magic"],
                          dict["relay_addr"],
                          dict["relay_port"],
                          dict["email"]);
    } else {
        connect(req, SIGNAL(cloneRepoSignal(QString &, bool)),
                this, SLOT(cloneRepoRequestFinished(QString &, bool)));

        req->cloneRepo(dict["repo_id"],
                       dict["relay_id"],
                       dict["repo_name"],
                       repo->localdir,
                       dict["token"],
                       repo->passwd,
                       dict["magic"],
                       dict["relay_addr"],
                       dict["relay_port"],
                       dict["email"]);
    }
}

void ListAccountReposDialog::onDownloadApiFailed(int code, ServerRepo *repo)
{
    qDebug() << __func__ << repo->name << "(" <<repo->id <<")";
    delete repo->req;
    repo->req = NULL;
}

void ListAccountReposDialog::downloadRepoRequestFinished(QString &repoId, bool result)
{
    qDebug() << __func__ << repoId << ", result:"<< result;
}

void ListAccountReposDialog::cloneRepoRequestFinished(QString &repoId, bool result)
{
    qDebug() << __func__ << repoId << ", result:"<< result;
}
