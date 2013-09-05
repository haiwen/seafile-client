#include <QtGui>

#include "QtAwesome.h"
#include "api/requests.h"
#include "seafile-applet.h"
#include "account-mgr.h"
#include "server-repos-list-view.h"
#include "server-repos-list-model.h"
#include "switch-account-dialog.h"
#include "login-dialog.h"
#include "cloud-view.h"

namespace {

const int kRefreshReposInterval = 10000;

}

CloudView::CloudView(QWidget *parent)
    : QWidget(parent),
      in_refresh_(false)
{
    repos_list_ = new ServerReposListView;
    repos_model_ = new ServerReposListModel;
    repos_list_->setModel(repos_model_);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(repos_list_);

    setLayout(layout);

    refresh_timer_ = new QTimer(this);
    connect(refresh_timer_, SIGNAL(timeout()), this, SLOT(refreshRepos()));

    connect(seafApplet->accountManager(), SIGNAL(accountAdded(const Account&)),
            this, SLOT(setCurrentAccount(const Account&)));
}

void CloudView::setCurrentAccount(const Account& account)
{
    if (current_account_ != account) {
        current_account_ = account;
        refreshRepos();
    }
}

void CloudView::refreshRepos()
{
    if (in_refresh_) {
        return;
    }

    if (!hasAccount()) {
        return;
    }

    in_refresh_ = true;

    ListReposRequest *req = new ListReposRequest(current_account_);
    connect(req, SIGNAL(success(const std::vector<ServerRepo>&)),
            this, SLOT(refreshRepos(const std::vector<ServerRepo>&)));
    connect(req, SIGNAL(failed(int)), this, SLOT(refreshReposFailed()));
    req->send();
}

void CloudView::refreshRepos(const std::vector<ServerRepo>& repos)
{
    repos_model_->setRepos(repos);
    in_refresh_ = false;
}

void CloudView::refreshReposFailed()
{
    qDebug("failed to refresh repos\n");
    in_refresh_ = false;
}

bool CloudView::hasAccount()
{
    return current_account_.token.length() > 0;
}

void CloudView::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);

    refresh_timer_->start(kRefreshReposInterval);
    if (!hasAccount()) {
        // Have an account
        std::vector<Account> accounts = seafApplet->accountManager()->loadAccounts();
        if (!accounts.empty()) {
            current_account_ = accounts[0];
        }
        refreshRepos();
    } else {
        // No account yet
    }
}

void CloudView::hideEvent(QHideEvent *event) {
    QWidget::hideEvent(event);
    refresh_timer_->stop();
}

void CloudView::showSwitchAccountDialog()
{
    SwitchAccountDialog dialog(this);
    connect(&dialog, SIGNAL(accountSelected(const Account&)),
            this, SLOT(setCurrentAccount(const Account&)));
    dialog.exec();
}

void CloudView::showAddAccountDialog()
{
    LoginDialog dialog(this);
    dialog.exec();
}

void CloudView::deleteAccount()
{
}
