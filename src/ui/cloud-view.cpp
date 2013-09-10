#include <vector>
#include <QtGui>
#include <QToolButton>
#include <QWidgetAction>
#include <QTreeView>

#include "QtAwesome.h"
#include "api/requests.h"
#include "seafile-applet.h"
#include "account-mgr.h"
#include "login-dialog.h"
#include "repo-tree-view.h"
#include "repo-tree-model.h"
#include "repo-item-delegate.h"
#include "cloud-view.h"

namespace {

const int kRefreshReposInterval = 10000;

enum {
    INDEX_LOADING_VIEW = 0,
    INDEX_REPOS_VIEW
};

}

CloudView::CloudView(QWidget *parent)
    : QWidget(parent),
      in_refresh_(false),
      list_repo_req_(NULL)
{
    repos_tree_ = new RepoTreeView(this);
    repos_model_ = new RepoTreeModel;
    repos_model_->setTreeView(repos_tree_);

    repos_tree_->setModel(repos_model_);
    repos_tree_->setItemDelegate(new RepoItemDelegate);

    createLoadingView();

    QStackedLayout *stack = new QStackedLayout;
    stack->insertWidget(INDEX_LOADING_VIEW, loading_view_);
    stack->insertWidget(INDEX_REPOS_VIEW, repos_tree_);
    setLayout(stack);

    prepareAccountButtonMenu();

    refresh_timer_ = new QTimer(this);
    connect(refresh_timer_, SIGNAL(timeout()), this, SLOT(refreshRepos()));

    connect(seafApplet->accountManager(), SIGNAL(accountAdded(const Account&)),
            this, SLOT(setCurrentAccount(const Account&)));

    connect(seafApplet->accountManager(), SIGNAL(accountAdded(const Account&)),
            this, SLOT(updateAccountMenu()));

    connect(seafApplet->accountManager(), SIGNAL(accountRemoved(const Account&)),
            this, SLOT(updateAccountMenu()));
}

void CloudView::createLoadingView()
{
    loading_view_ = new QWidget(this);

    QVBoxLayout *layout = new QVBoxLayout;
    loading_view_->setLayout(layout);

    QMovie *gif = new QMovie(":/images/loading.gif");
    QLabel *label = new QLabel;
    label->setMovie(gif);
    label->setAlignment(Qt::AlignCenter);
    gif->start();

    layout->addWidget(label);
}

void CloudView::showLoadingView()
{
    QStackedLayout *stack = (QStackedLayout *)(layout());

    stack->setCurrentIndex(INDEX_LOADING_VIEW);
}

void CloudView::showRepos()
{
    QStackedLayout *stack = (QStackedLayout *)(layout());
    stack->setCurrentIndex(INDEX_REPOS_VIEW);
}

void CloudView::prepareAccountButtonMenu()
{
    account_menu_ = new QMenu;

    account_tool_button_ = new QToolButton(this);
    account_tool_button_->setMenu(account_menu_);
    account_tool_button_->setPopupMode(QToolButton::InstantPopup);
    account_tool_button_->setIcon(QIcon(":/images/account.png"));

    account_widget_action_ = new QWidgetAction(this);
    account_widget_action_->setDefaultWidget(account_tool_button_);

    updateAccountMenu();
}

/**
 * Update the account menu when accounts changed
 */
void CloudView::updateAccountMenu()
{
    // Remove all menu items
    account_menu_->clear();

    // Add accounts again
    const std::vector<Account>& accounts = seafApplet->accountManager()->accounts();

    if (!accounts.empty()) {
        if (!hasAccount()) {
            setCurrentAccount(accounts[0]);
        }

        for (int i = 0, n = accounts.size(); i < n; i++) {
            Account account = accounts[i];
            QAction *action = makeAccountAction(accounts[i]);
            if (account == current_account_) {
                // Add a check sign before current account
                action->setCheckable(true);
                action->setChecked(true);
            }
            account_menu_->addAction(action);
        }

        account_menu_->addSeparator();
    }

    // Add rest items
    add_account_action_ = new QAction(tr("Add an account"), this);
    delete_account_action_ = new QAction(tr("Delete this account"), this);

    connect(add_account_action_, SIGNAL(triggered()), this, SLOT(showAddAccountDialog()));
    connect(delete_account_action_, SIGNAL(triggered()), this, SLOT(deleteAccount()));

    account_menu_->addAction(add_account_action_);
    if (hasAccount()) {
        account_menu_->addAction(delete_account_action_);
    }
}

void CloudView::setCurrentAccount(const Account& account)
{
    if (current_account_ != account) {
        current_account_ = account;
        in_refresh_ = false;
        repos_model_->clear();
        showLoadingView();
        refreshRepos();

        qDebug("switch to account %s\n", account.username.toUtf8().data());
    }
}

QAction* CloudView::makeAccountAction(const Account& account)
{
    QString text = account.username + "(" + account.serverUrl.host() + ")";
    QAction *action = new QAction(text, account_menu_);
    action->setData(QVariant::fromValue(account));
    // QMenu won't display tooltip for menu item
    // action->setToolTip(account.serverUrl.host());

    connect(action, SIGNAL(triggered()), this, SLOT(onAccountItemClicked()));

    return action;
}

// Switch to the clicked account in the account menu
void CloudView::onAccountItemClicked()
{
    QAction *action = (QAction *)(sender());
    Account account = qvariant_cast<Account>(action->data());

    if (account == current_account_) {
        return;
    }

    setCurrentAccount(account);
    updateAccountMenu();
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

    if (list_repo_req_) {
        delete list_repo_req_;
    }
    list_repo_req_ = new ListReposRequest(current_account_);
    connect(list_repo_req_, SIGNAL(success(const std::vector<ServerRepo>&)),
            this, SLOT(refreshRepos(const std::vector<ServerRepo>&)));
    connect(list_repo_req_, SIGNAL(failed(int)), this, SLOT(refreshReposFailed()));
    list_repo_req_->send();
}

void CloudView::refreshRepos(const std::vector<ServerRepo>& repos)
{
    in_refresh_ = false;
    repos_model_->setRepos(repos);

    list_repo_req_->deleteLater();
    list_repo_req_ = NULL;

    showRepos();
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
}

void CloudView::hideEvent(QHideEvent *event) {
    QWidget::hideEvent(event);
    refresh_timer_->stop();
}

void CloudView::showAddAccountDialog()
{
    LoginDialog dialog(this);
    dialog.exec();
}

void CloudView::deleteAccount()
{
    QString question = tr("Are you sure to remove this account?");
    if (QMessageBox::question(this, tr("Seafile"), question) == QMessageBox::Ok) {
        seafApplet->accountManager()->removeAccount(current_account_);
        setCurrentAccount(Account());
    }
}

