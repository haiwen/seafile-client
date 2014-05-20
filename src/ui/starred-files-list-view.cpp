#include <QtGui>
#include <QHeaderView>
#include <QDesktopServices>
#include <QEvent>
#include <QShowEvent>
#include <QHideEvent>

#include "QtAwesome.h"
#include "utils/utils.h"
#include "seafile-applet.h"
#include "account-mgr.h"
#include "rpc/rpc-client.h"
#include "rpc/local-repo.h"
#include "starred-file-item.h"
#include "starred-files-list-model.h"

#include "starred-files-list-view.h"


StarredFilesListView::StarredFilesListView(QWidget *parent)
    : QListView(parent)
{
#ifdef Q_WS_MAC
    setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif

    createActions();
}

void StarredFilesListView::createActions()
{
    open_file_action_ = new QAction(tr("&Open"), this);
    open_file_action_->setIcon(QIcon(":/images/folder-open.png"));
    open_file_action_->setIconVisibleInMenu(true);
    open_file_action_->setStatusTip(tr("Open this file"));
    connect(open_file_action_, SIGNAL(triggered()), this, SLOT(openLocalFile()));

    view_file_on_web_action_ = new QAction(tr("view on &Web"), this);
    view_file_on_web_action_->setIcon(QIcon(":/images/cloud.png"));
    view_file_on_web_action_->setIconVisibleInMenu(true);
    view_file_on_web_action_->setStatusTip(tr("view this file on seahub"));
    connect(view_file_on_web_action_, SIGNAL(triggered()), this, SLOT(viewFileOnWeb()));
}

void StarredFilesListView::openLocalFile()
{
    StarredFile file = qvariant_cast<StarredFile>(view_file_on_web_action_->data());

    LocalRepo r;

    seafApplet->rpcClient()->getLocalRepo(file.repo_id, &r);

    if (r.isValid()) {
        printf ("we have the local repo for file %s\n", file.path.toUtf8().data());

        QString path = QDir(r.worktree).filePath(file.path.mid(1));

        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    } else {
        printf ("we do not have the local repo for file %s\n", file.path.toUtf8().data());
    }
}

void StarredFilesListView::viewFileOnWeb()
{
    StarredFile file = qvariant_cast<StarredFile>(view_file_on_web_action_->data());

    const Account& account = seafApplet->accountManager()->currentAccount();
    if (account.isValid()) {
        QUrl url = account.serverUrl;
        url.setPath(url.path() + "/repo/" + file.repo_id + "/files/");
        url.addQueryItem("p", file.path);

        QDesktopServices::openUrl(url);
    }
}

void StarredFilesListView::updateActions()
{
    StarredFileItem *item = NULL;
    QItemSelection selected = selectionModel()->selection();
    QModelIndexList indexes = selected.indexes();
    if (indexes.size() != 0) {
        const QModelIndex& index = indexes.at(0);
        QStandardItem *it = ((StarredFilesListModel *)model())->itemFromIndex(index);
        item = (StarredFileItem *)it;
    }

    if (!item) {
        // No item is selected
        open_file_action_->setEnabled(false);
        view_file_on_web_action_->setEnabled(false);
        return;
    }

    const StarredFile& file = item->file();

    open_file_action_->setData(QVariant::fromValue(file));
    view_file_on_web_action_->setData(QVariant::fromValue(file));
}

void StarredFilesListView::contextMenuEvent(QContextMenuEvent *event)
{
    QPoint pos = event->pos();
    QModelIndex index = indexAt(pos);
    if (!index.isValid()) {
        // Not clicked at a repo item
        return;
    }

    QStandardItem *item = getFileItem(index);
    if (!item) {
        return;
    }
    updateActions();
    QMenu *menu = prepareContextMenu((StarredFileItem *)item);
    pos = viewport()->mapToGlobal(pos);
    menu->exec(pos);
}

QStandardItem*
StarredFilesListView::getFileItem(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return NULL;
    }
    const StarredFilesListModel *model = (const StarredFilesListModel*)index.model();
    QStandardItem *item = model->itemFromIndex(index);
    return item;
}

QMenu* StarredFilesListView::prepareContextMenu(const StarredFileItem *item)
{
    QMenu *menu = new QMenu(this);

    menu->addAction(open_file_action_);
    menu->addAction(view_file_on_web_action_);

    return menu;
}
