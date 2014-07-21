extern "C" {

#include <ccnet/peer.h>

}

#include <vector>
#include <QtGlobal>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QtWidgets>
#else
#include <QtGui>
#endif
#include <QWidget>

#include "QtAwesome.h"
#include "seafile-applet.h"
#include "rpc/rpc-client.h"
#include "account-mgr.h"
#include "create-repo-dialog.h"
#include "clone-tasks-dialog.h"
#include "server-status-dialog.h"
#include "main-window.h"
#include "repos-tab.h"
#include "starred-files-tab.h"
#include "activities-tab.h"
#include "account-view.h"
#include "seafile-tab-widget.h"
#include "utils/paint-utils.h"

#include "cloud-view.h"

namespace {

const int kRefreshStatusInterval = 1000;

const int kIndexOfAccountView = 1;
const int kIndexOfToolBar = 2;
const int kIndexOfTabWidget = 3;

enum {
    TAB_INDEX_REPOS = 0,
    TAB_INDEX_STARRED_FILES,
    TAB_INDEX_ACTIVITIES,
};


}

CloudView::CloudView(QWidget *parent)
    : QWidget(parent),
      clone_task_dialog_(NULL)

{
    setupUi(this);

    // seahub_messages_monitor_ = new SeahubMessagesMonitor(this);
    // mSeahubMessagesBtn->setVisible(false);

    layout()->setContentsMargins(1, 0, 1, 0);

    // Setup widgets from top down
    setupHeader();

    createAccountView();

    createTabs();

    // tool bar have to be created after tabs, since some of the toolbar
    // actions are provided by the tabs
    createToolBar();

    setupDropArea();

    setupFooter();

    QVBoxLayout *vlayout = (QVBoxLayout *)layout();
    vlayout->insertWidget(kIndexOfAccountView, account_view_);
    vlayout->insertWidget(kIndexOfToolBar, tool_bar_);
    vlayout->insertWidget(kIndexOfTabWidget, tabs_);

    resizer_ = new QSizeGrip(this);
    resizer_->resize(resizer_->sizeHint());

    refresh_status_bar_timer_ = new QTimer(this);
    connect(refresh_status_bar_timer_, SIGNAL(timeout()), this, SLOT(refreshStatusBar()));

    AccountManager *account_mgr = seafApplet->accountManager();
    connect(account_mgr, SIGNAL(accountsChanged()),
            this, SLOT(onAccountChanged()));

#if defined(Q_OS_MAC)
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


void CloudView::createAccountView()
{
    account_view_ = new AccountView;
}

void CloudView::createTabs()
{
    tabs_ = new SeafileTabWidget;
    // tabs_ = new QTabWidget;

    repos_tab_ = new ReposTab;

    QString base_icon_path = ":/images/tabs/";
    tabs_->addTab(repos_tab_, tr("Libraries"), base_icon_path + "repos.png");

    starred_files_tab_ = new StarredFilesTab;
    tabs_->addTab(starred_files_tab_, tr("Starred"), base_icon_path + "starred.png");

    activities_tab_ = new ActivitiesTab;

    connect(tabs_, SIGNAL(currentTabChanged(int)),
            this, SLOT(onTabChanged(int)));

    connect(activities_tab_, SIGNAL(activitiesSupported()),
            this, SLOT(addActivitiesTab()));
}

void CloudView::addActivitiesTab()
{
    if (tabs_->count() < 3) {
        QString base_icon_path = ":/images/tabs/";
        tabs_->addTab(activities_tab_, tr("Activities"), base_icon_path + "activities.png");
        tabs_->adjustTabsWidth(rect().width());
    }
}

void CloudView::setupDropArea()
{
    mDropArea->setAcceptDrops(true);
    mDropArea->installEventFilter(this);
    connect(mSelectFolderBtn, SIGNAL(clicked()), this, SLOT(chooseFolderToSync()));
}

void CloudView::setupFooter()
{
    // mDownloadTasksInfo->setText("0");
    // mDownloadTasksBtn->setIcon(QIcon(":/images/download-gray.png"));
    // mDownloadTasksBtn->setToolTip(tr("Show download tasks"));
    // connect(mDownloadTasksBtn, SIGNAL(clicked()), this, SLOT(showCloneTasksDialog()));

    mServerStatusBtn->setIcon(QIcon(":/images/link-green.png"));
    int w = ::getDPIScaledSize(18);
    mServerStatusBtn->setIconSize(QSize(w, w));
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
#if defined(Q_OS_WIN)
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
    const std::vector<Account>& accounts = seafApplet->accountManager()->accounts();
    if (accounts.empty()) {
        return;
    }
    CreateRepoDialog dialog(accounts[0], path, this);
    if (dialog.exec() == QDialog::Accepted) {
        repos_tab_->refresh();
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

bool CloudView::hasAccount()
{
    return seafApplet->accountManager()->hasAccount();
}

void CloudView::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);

    refresh_status_bar_timer_->start(kRefreshStatusInterval);
}

void CloudView::hideEvent(QHideEvent *event) {
    QWidget::hideEvent(event);
    refresh_status_bar_timer_->stop();
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

    int w = ::getDPIScaledSize(24);
    tool_bar_->setIconSize(QSize(w, w));

    std::vector<QAction*> repo_actions = repos_tab_->getToolBarActions();
    for (int i = 0, n = repo_actions.size(); i < n; i++) {
        QAction *action = repo_actions[i];
        tool_bar_->addAction(action);
    }

    QWidget *spacer = new QWidget;
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    tool_bar_->addWidget(spacer);

    refresh_action_ = new QAction(tr("Refresh"), this);
    refresh_action_->setIcon(QIcon(":/images/refresh.png"));
    refresh_action_->setEnabled(hasAccount());
    connect(refresh_action_, SIGNAL(triggered()), this, SLOT(onRefreshClicked()));
    tool_bar_->addAction(refresh_action_);

    QWidget *spacer_right = new QWidget;
    spacer_right->setObjectName("spacerWidget");
    spacer_right->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    tool_bar_->addWidget(spacer_right);
}

void CloudView::onRefreshClicked()
{
    if (tabs_->currentIndex() == TAB_INDEX_REPOS) {
        repos_tab_->refresh();
    } else if (tabs_->currentIndex() == TAB_INDEX_STARRED_FILES) {
        starred_files_tab_->refresh();
    } else if (tabs_->currentIndex() == TAB_INDEX_ACTIVITIES) {
        activities_tab_->refresh();
    }
}

void CloudView::resizeEvent(QResizeEvent *event)
{
    resizer_->move(rect().bottomRight() - resizer_->rect().bottomRight());
    resizer_->raise();
    tabs_->adjustTabsWidth(rect().width());
}

void CloudView::onAccountChanged()
{
    refresh_action_->setEnabled(hasAccount());

    tabs_->removeTab(2, activities_tab_);
    tabs_->adjustTabsWidth(rect().width());

    repos_tab_->refresh();
    starred_files_tab_->refresh();
    activities_tab_->refresh();

    account_view_->onAccountChanged();
}

void CloudView::onTabChanged(int index)
{
    bool drop_area_visible = index == 0;
    mDropArea->setVisible(drop_area_visible);
}
