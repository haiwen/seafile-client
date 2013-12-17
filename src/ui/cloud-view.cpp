extern "C" {

#include <ccnet/peer.h>

}

#include <vector>
#include <QtGui>
#include <QToolButton>
#include <QWidgetAction>
#include <QTreeView>

#include "QtAwesome.h"
#include "seahub-messages-monitor.h"
#include "api/requests.h"
#include "seafile-applet.h"
#include "rpc/rpc-client.h"
#include "account-mgr.h"
#include "configurator.h"
#include "login-dialog.h"
#include "create-repo-dialog.h"
#include "repo-tree-view.h"
#include "repo-tree-model.h"
#include "repo-item-delegate.h"
#include "clone-tasks-dialog.h"
#include "server-status-dialog.h"
#include "main-window.h"
#include "cloud-view.h"

namespace {

const int kRefreshReposInterval = 1000 * 60 * 5; // 5 min
const int kRefreshStatusInterval = 1000;

enum {
    INDEX_LOADING_VIEW = 0,
    INDEX_REPOS_VIEW
};

}

CloudView::CloudView(QWidget *parent)
    : QWidget(parent),
      in_refresh_(false),
      list_repo_req_(NULL),
      clone_task_dialog_(NULL)

{
    setupUi(this);

    seahub_messages_monitor_ = new SeahubMessagesMonitor(this);

    setupHeader();
    createRepoModelView();
    createLoadingView();
    mStack->insertWidget(INDEX_LOADING_VIEW, loading_view_);
    mStack->insertWidget(INDEX_REPOS_VIEW, repos_tree_);

    createToolBar();
    updateAccountInfoDisplay();
    prepareAccountButtonMenu();

    prepareDropArea();
    prepareFooter();

    refresh_status_bar_timer_ = new QTimer(this);
    connect(refresh_status_bar_timer_, SIGNAL(timeout()), this, SLOT(refreshStatusBar()));

    refresh_timer_ = new QTimer(this);
    connect(refresh_timer_, SIGNAL(timeout()), this, SLOT(refreshRepos()));

    connect(seafApplet->accountManager(), SIGNAL(accountAdded(const Account&)),
            this, SLOT(setCurrentAccount(const Account&)));

    connect(seafApplet->accountManager(), SIGNAL(accountAdded(const Account&)),
            this, SLOT(updateAccountMenu()));

    connect(seafApplet->accountManager(), SIGNAL(accountRemoved(const Account&)),
            this, SLOT(updateAccountMenu()));
}

void CloudView::setupHeader()
{
    mMinimizeBtn->setText("");
    mMinimizeBtn->setIcon(awesome->icon(icon_minus));
    connect(mMinimizeBtn, SIGNAL(clicked()), this, SLOT(onMinimizeBtnClicked()));

    mCloseBtn->setText("");
    mCloseBtn->setIcon(awesome->icon(icon_remove));
    connect(mCloseBtn, SIGNAL(clicked()), this, SLOT(onCloseBtnClicked()));

    // Handle mouse move event
    mHeader->installEventFilter(this);
}

void CloudView::prepareFooter()
{
    mDownloadTasksInfo->setText("0");
    mDownloadTasksBtn->setIcon(awesome->icon(icon_download_alt));
    mDownloadTasksBtn->setToolTip(tr("Show download tasks"));
    connect(mDownloadTasksBtn, SIGNAL(clicked()), this, SLOT(showCloneTasksDialog()));

    mServerStatusBtn->setIcon(awesome->icon(icon_lightbulb));
    connect(mServerStatusBtn, SIGNAL(clicked()), this, SLOT(showServerStatusDialog()));

    mDownloadRateArrow->setText(QChar(icon_arrow_down));
    mDownloadRateArrow->setFont(awesome->font(16));
    mDownloadRate->setText("0 kB/s");
    mDownloadRate->setToolTip(tr("current download rate"));

    mUploadRateArrow->setText(QChar(icon_arrow_up));
    mUploadRateArrow->setFont(awesome->font(16));
    mUploadRate->setText("0 kB/s");
    mUploadRate->setToolTip(tr("current upload rate"));
}

void CloudView::prepareDropArea()
{
    connect(mSelectFolderBtn, SIGNAL(clicked()), this, SLOT(chooseFolderToSync()));
}

void CloudView::chooseFolderToSync()
{
    QString msg = tr("Please Choose a folder to sync");
    QString dir = QFileDialog::getExistingDirectory(this, msg,
                                                    seafApplet->configurator()->worktreeDir(),
                                                    QFileDialog::ShowDirsOnly
                                                    | QFileDialog::DontResolveSymlinks);
    if (dir.isEmpty()) {
        return;
    }
}

void CloudView::onMinimizeBtnClicked()
{
    seafApplet->mainWindow()->showMinimized();
}

void CloudView::onCloseBtnClicked()
{
    seafApplet->mainWindow()->hide();
}

bool CloudView::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == mHeader) {
        static QPoint oldPos;
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *ev = (QMouseEvent *)event;
            oldPos = ev->globalPos();

            return true;

        } else if (event->type() == QEvent::MouseMove) {
            QMouseEvent *ev = (QMouseEvent *)event;
            const QPoint delta = ev->globalPos() - oldPos;

            MainWindow *win = seafApplet->mainWindow();
            win->move(win->x() + delta.x(), win->y() + delta.y());

            oldPos = ev->globalPos();
            return true;
        }
    }

    return QWidget::eventFilter(obj, event);
}

CloneTasksDialog *CloudView::cloneTasksDialog()
{
    if (clone_task_dialog_ == NULL) {
        clone_task_dialog_ = new CloneTasksDialog;
    }

    return clone_task_dialog_;
}

void CloudView::createRepoModelView()
{
    repos_tree_ = new RepoTreeView(this);
    repos_model_ = new RepoTreeModel;
    repos_model_->setTreeView(repos_tree_);

    repos_tree_->setModel(repos_model_);
    repos_tree_->setItemDelegate(new RepoItemDelegate);
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

    mStack->setCurrentIndex(INDEX_LOADING_VIEW);

    mDropArea->setVisible(false);
}

void CloudView::showRepos()
{
    mStack->setCurrentIndex(INDEX_REPOS_VIEW);

    mDropArea->setVisible(true);
}

void CloudView::prepareAccountButtonMenu()
{
    account_menu_ = new QMenu;
    mAccountBtn->setMenu(account_menu_);

    mAccountBtn->setPopupMode(QToolButton::InstantPopup);

    updateAccountMenu();
}

void CloudView::updateAccountInfoDisplay()
{
    mAccountBtn->setIcon(QIcon(":/images/account.png"));
    mAccountBtn->setIconSize(QSize(32, 32));
    if (hasAccount()) {
        mEmail->setText(current_account_.username);
        mServerAddr->setText(current_account_.serverUrl.host());
    } else {
        mEmail->setText(tr("No account"));
        mServerAddr->setText(QString());
    }
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
                action->setChecked(true);
            }
            account_menu_->addAction(action);
        }

        account_menu_->addSeparator();
    }

    // Add rest items
    add_account_action_ = new QAction(tr("Add an account"), this);
    add_account_action_->setIcon(awesome->icon(icon_plus));
    add_account_action_->setIconVisibleInMenu(true);
    connect(add_account_action_, SIGNAL(triggered()), this, SLOT(showAddAccountDialog()));
    account_menu_->addAction(add_account_action_);

    if (hasAccount()) {
        delete_account_action_ = new QAction(tr("Delete this account"), this);
        delete_account_action_->setIcon(awesome->icon(icon_remove));
        delete_account_action_->setIconVisibleInMenu(true);
        connect(delete_account_action_, SIGNAL(triggered()), this, SLOT(deleteAccount()));
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

        seahub_messages_monitor_->refresh();

        updateAccountInfoDisplay();
        if (account.isValid()) {
            seafApplet->accountManager()->updateAccountLastVisited(account);
        }
        qDebug("switch to account %s\n", account.username.toUtf8().data());
    }

    refresh_action_->setEnabled(account.isValid());
}

QAction* CloudView::makeAccountAction(const Account& account)
{
    QString text = account.username + "(" + account.serverUrl.host() + ")";
    QAction *action = new QAction(text, account_menu_);
    action->setData(QVariant::fromValue(account));
    action->setCheckable(true);
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
    refresh_status_bar_timer_->start(kRefreshStatusInterval);
}

void CloudView::hideEvent(QHideEvent *event) {
    QWidget::hideEvent(event);
    refresh_timer_->stop();
    refresh_status_bar_timer_->stop();
}

void CloudView::showAddAccountDialog()
{
    LoginDialog dialog(this);
    dialog.exec();
}

void CloudView::deleteAccount()
{
    QString question = tr("Are you sure to remove this account?<br>"
                          "<b>Warning: All libraries of this account would be unsynced!</b>");
    if (QMessageBox::question(this,
                              tr(SEAFILE_CLIENT_BRAND),
                              question,
                              QMessageBox::Ok | QMessageBox::Cancel,
                              QMessageBox::Cancel) == QMessageBox::Ok) {

        QString error, server_addr = current_account_.serverUrl.host();
        if (seafApplet->rpcClient()->unsyncReposByAccount(server_addr,
                                                          current_account_.username,
                                                          &error) < 0) {
            QMessageBox::warning(this, tr(SEAFILE_CLIENT_BRAND),
                                 tr("Failed to unsync libraries of this account: %1").arg(error),
                                 QMessageBox::Ok);
        }

        Account account = current_account_;
        setCurrentAccount(Account());
        seafApplet->accountManager()->removeAccount(account);
    }
}

void CloudView::refreshTasksInfo()
{
    int count = 0;
    if (seafApplet->rpcClient()->getCloneTasksCount(&count) < 0) {
        return;
    }

    mDownloadTasksInfo->setText(QString::number(count));
}

void CloudView::refreshServerStatus()
{
    GList *servers = NULL;
    if (seafApplet->rpcClient()->getServers(&servers) < 0) {
        qDebug("failed to get ccnet servers list\n");
        return;
    }

    if (!servers) {
        mServerStatusBtn->setIcon(awesome->icon(icon_lightbulb));
        mServerStatusBtn->setToolTip(tr("no server connected"));
        return;
    }

    GList *ptr;
    bool all_server_connected = true;
    bool all_server_disconnected = true;
    for (ptr = servers; ptr ; ptr = ptr->next) {
        CcnetPeer *server = (CcnetPeer *)ptr->data;
        if (server->net_state == PEER_CONNECTED) {
            all_server_disconnected = false;
        } else {
            all_server_connected = false;
        }
    }

    QColor color;
    QString tool_tip;
    if (all_server_connected) {
        color = "green";
        tool_tip = tr("all servers connected");
    } else if (all_server_disconnected) {
        color = "red";
        tool_tip = tr("no server connected");
    } else {
        color = "red";
        tool_tip = tr("some servers not connected");
    }
    mServerStatusBtn->setIcon(awesome->icon(icon_lightbulb, color));
    mServerStatusBtn->setToolTip(tool_tip);

    g_list_foreach (servers, (GFunc)g_object_unref, NULL);
    g_list_free (servers);
}

void CloudView::refreshTransferRate()
{
    int up_rate, down_rate;
    if (seafApplet->rpcClient()->getUploadRate(&up_rate) < 0) {
        return;
    }

    if (seafApplet->rpcClient()->getDownloadRate(&down_rate) < 0) {
        return;
    }

    mUploadRate->setText(tr("%1 kB/s").arg(up_rate / 1024));
    mDownloadRate->setText(tr("%1 kB/s").arg(down_rate / 1024));
}

void CloudView::refreshStatusBar()
{
    if (!seafApplet->mainWindow()->isVisible()) {
        return;
    }
    refreshTasksInfo();
    refreshServerStatus();
    refreshTransferRate();
}

void CloudView::showCloneTasksDialog()
{
    //CloneTasksDialog dialog(this);
    if (clone_task_dialog_ == NULL) {
        clone_task_dialog_ = new CloneTasksDialog;
    }

    clone_task_dialog_->updateTasks();
    clone_task_dialog_->show();
    clone_task_dialog_->raise();
    clone_task_dialog_->activateWindow();
}

void CloudView::showServerStatusDialog()
{
    ServerStatusDialog dialog(this);
    dialog.exec();
}

void CloudView::createToolBar()
{
    tool_bar_ = new QToolBar;

    QVBoxLayout *vlayout = (QVBoxLayout *)layout();
    vlayout->insertWidget(2, tool_bar_);

    std::vector<QAction*> repo_actions = repos_tree_->getToolBarActions();
    for (int i = 0, n = repo_actions.size(); i < n; i++) {
        QAction *action = repo_actions[i];
        tool_bar_->addAction(action);
        action->setEnabled(hasAccount());
    }

    QWidget *spacerWidget = new QWidget;
	spacerWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	spacerWidget->setVisible(true);
	tool_bar_->addWidget(spacerWidget);

    refresh_action_ = new QAction(tr("Refresh"), this);
    refresh_action_->setIcon(QIcon(":/images/refresh.png"));
    refresh_action_->setEnabled(hasAccount());
    connect(refresh_action_, SIGNAL(triggered()), this, SLOT(onRefreshClicked()));
    tool_bar_->addAction(refresh_action_);
}

void CloudView::onRefreshClicked()
{
    if (hasAccount()) {
        showLoadingView();
        refreshRepos();
    }
}
