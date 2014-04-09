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
#include "api/api-error.h"
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
#include "init-vdrive-dialog.h"
#include "main-window.h"
#include "cloud-view.h"

namespace {

const int kRefreshReposInterval = 1000 * 60 * 5; // 5 min
const int kRefreshStatusInterval = 1000;
const char *kLoadingFaieldLabelName = "loadingFailedText";

enum {
    INDEX_LOADING_VIEW = 0,
    INDEX_LOADING_FAILED_VIEW,
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

    // seahub_messages_monitor_ = new SeahubMessagesMonitor(this);
    mSeahubMessagesBtn->setVisible(false);

    setupHeader();
    createRepoModelView();
    createLoadingView();
    createLoadingFailedView();
    mStack->insertWidget(INDEX_LOADING_VIEW, loading_view_);
    mStack->insertWidget(INDEX_LOADING_FAILED_VIEW, loading_failed_view_);
    mStack->insertWidget(INDEX_REPOS_VIEW, repos_tree_);

    createToolBar();
    updateAccountInfoDisplay();
    prepareAccountButtonMenu();

    setupDropArea();
    setupFooter();

    resizer_ = new QSizeGrip(this);
    resizer_->resize(resizer_->sizeHint());

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
#ifdef Q_WS_MAC
    mHeader->setVisible(false);
#endif
}

void CloudView::setupHeader()
{
    mLogo->setText("");
    mLogo->setToolTip(getBrand());
    mLogo->setPixmap(QPixmap(":/images/seafile-24.png"));

    mBrand->setText(getBrand());
    mBrand->setToolTip(getBrand());

    mMinimizeBtn->setText("");
    mMinimizeBtn->setToolTip(tr("Minimize"));
    mMinimizeBtn->setIcon(awesome->icon(icon_minus, QColor("#808081")));
    connect(mMinimizeBtn, SIGNAL(clicked()), this, SLOT(onMinimizeBtnClicked()));

    mCloseBtn->setText("");
    mCloseBtn->setToolTip(tr("Close"));
    mCloseBtn->setIcon(awesome->icon(icon_remove, QColor("#808081")));
    connect(mCloseBtn, SIGNAL(clicked()), this, SLOT(onCloseBtnClicked()));

    // Handle mouse move event
    mHeader->installEventFilter(this);
}

void CloudView::setupFooter()
{
    // mDownloadTasksInfo->setText("0");
    // mDownloadTasksBtn->setIcon(QIcon(":/images/download-gray.png"));
    // mDownloadTasksBtn->setToolTip(tr("Show download tasks"));
    // connect(mDownloadTasksBtn, SIGNAL(clicked()), this, SLOT(showCloneTasksDialog()));

    mServerStatusBtn->setIcon(QIcon(":/images/link-green"));
    connect(mServerStatusBtn, SIGNAL(clicked()), this, SLOT(showServerStatusDialog()));

    // mDownloadRateArrow->setText(QChar(icon_arrow_down));
    // mDownloadRateArrow->setFont(awesome->font(16));
    mDownloadRateArrow->setPixmap(QPixmap(":/images/arrow-down.png"));
    mDownloadRate->setText("0 kB/s");
    mDownloadRate->setToolTip(tr("current download rate"));

    // mUploadRateArrow->setText(QChar(icon_arrow_up));
    // mUploadRateArrow->setFont(awesome->font(16));
    mUploadRateArrow->setPixmap(QPixmap(":/images/arrow-up.png"));
    mUploadRate->setText("0 kB/s");
    mUploadRate->setToolTip(tr("current upload rate"));
}

void CloudView::chooseFolderToSync()
{
    QString msg = tr("Please Choose a folder to sync");
#if defined(Q_WS_WIN)
    QString parent_dir = "C:\\";
#else
    QString parent_dir = QDir::homePath();
#endif
    QString dir = QFileDialog::getExistingDirectory(this, msg, parent_dir,
                                                    QFileDialog::ShowDirsOnly
                                                    | QFileDialog::DontResolveSymlinks);
    if (dir.isEmpty()) {
        return;
    }

    showCreateRepoDialog(dir);
}

void CloudView::showCreateRepoDialog(const QString& path)
{
    CreateRepoDialog dialog(current_account_, path, this);
    if (dialog.exec() == QDialog::Accepted) {
        refreshRepos();
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

void CloudView::setupDropArea()
{
    mDropArea->setAcceptDrops(true);
    mDropArea->installEventFilter(this);
    connect(mSelectFolderBtn, SIGNAL(clicked()), this, SLOT(chooseFolderToSync()));
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

    } else if (obj == mDropArea) {
        if (event->type() == QEvent::DragEnter) {
            QDragEnterEvent *ev = (QDragEnterEvent *)event;
            if (ev->mimeData()->hasUrls() && ev->mimeData()->urls().size() == 1) {
                const QUrl url = ev->mimeData()->urls().at(0);
                if (url.scheme() == "file") {
                    QString path = url.toLocalFile();
                    if (QFileInfo(path).isDir()) {
                        ev->acceptProposedAction();
                    }
                }
            }
            return true;
        } else if (event->type() == QEvent::Drop) {
            QDropEvent *ev = (QDropEvent *)event;
            const QUrl url = ev->mimeData()->urls().at(0);
            QString path = url.toLocalFile();
            showCreateRepoDialog(path);
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

void CloudView::createLoadingFailedView()
{
    loading_failed_view_ = new QWidget(this);

    QVBoxLayout *layout = new QVBoxLayout;
    loading_failed_view_->setLayout(layout);

    QLabel *label = new QLabel;
    label->setObjectName(kLoadingFaieldLabelName);
    QString link = QString("<a style=\"color:#777\" href=\"#\">%1</a>").arg(tr("retry"));
    QString label_text = tr("Failed to get libraries information<br/>"
                            "Please %1").arg(link);
    label->setText(label_text);
    label->setAlignment(Qt::AlignCenter);

    connect(label, SIGNAL(linkActivated(const QString&)),
            this, SLOT(onRefreshClicked()));
    label->installEventFilter(this);

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
        mServerAddr->setOpenExternalLinks(true);
        mServerAddr->setToolTip(tr("click to open the website"));

        QString host = current_account_.serverUrl.host();
        QString href = current_account_.serverUrl.toString();
        QString text = QString("<a style="
                               "\"color:#A4A4A4; text-decoration: none;\" "
                               "href=\"%1\">%2</a>").arg(href).arg(host);

        mServerAddr->setText(text);
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

        // seahub_messages_monitor_->refresh();

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
    connect(list_repo_req_, SIGNAL(failed(const ApiError&)),
            this, SLOT(refreshReposFailed(const ApiError&)));
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

void CloudView::refreshReposFailed(const ApiError& error)
{
    qDebug("failed to refresh repos");
    in_refresh_ = false;

    if (mStack->currentIndex() == INDEX_LOADING_VIEW) {
        mStack->setCurrentIndex(INDEX_LOADING_FAILED_VIEW);
    }
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
    // Show InitVirtualDriveDialog for the first account added
    AccountManager *account_mgr = seafApplet->accountManager();
    if (dialog.exec() == QDialog::Accepted
        && account_mgr->accounts().size() == 1) {

        const Account& account = account_mgr->accounts()[0];
        InitVirtualDriveDialog dialog(account, seafApplet->mainWindow());
        dialog.exec();
    }
}

void CloudView::deleteAccount()
{
    QString question = tr("Are you sure to remove this account?<br>"
                          "<b>Warning: All libraries of this account would be unsynced!</b>");
    if (QMessageBox::question(this,
                              getBrand(),
                              question,
                              QMessageBox::Ok | QMessageBox::Cancel,
                              QMessageBox::Cancel) == QMessageBox::Ok) {

        QString error, server_addr = current_account_.serverUrl.host();
        if (seafApplet->rpcClient()->unsyncReposByAccount(server_addr,
                                                          current_account_.username,
                                                          &error) < 0) {
            QMessageBox::warning(this, getBrand(),
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

    // mDownloadTasksInfo->setText(QString::number(count));
}

void CloudView::refreshServerStatus()
{
    GList *servers = NULL;
    if (seafApplet->rpcClient()->getServers(&servers) < 0) {
        qDebug("failed to get ccnet servers list\n");
        return;
    }

    if (!servers) {
        mServerStatusBtn->setIcon(QIcon(":/images/link-green"));
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

    bool all_connected = false;
    QString tool_tip;
    if (all_server_connected) {
        all_connected = true;
        tool_tip = tr("all servers connected");
    } else if (all_server_disconnected) {
        tool_tip = tr("no server connected");
    } else {
        tool_tip = tr("some servers not connected");
    }
    mServerStatusBtn->setIcon(QIcon(all_connected
                                    ? ":/images/link-green.png"
                                    : ":/images/link-red.png"));
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

void CloudView::resizeEvent(QResizeEvent *event)
{
    resizer_->move(rect().bottomRight() - resizer_->rect().bottomRight());
    resizer_->raise();
}
