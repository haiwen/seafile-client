#include <QHeaderView>
#include <QPainter>

#include "QtAwesome.h"
#include "repo-item.h"
#include "repo-tree-model.h"
#include "repo-tree-view.h"

namespace {

const int kSyncStatusIconSize = 16;

}

RepoTreeView::RepoTreeView(QWidget *parent)
    : QTreeView(parent)
{
    header()->hide();
    setMouseTracking(true);
}

void RepoTreeView::drawBranches(QPainter *painter,
                                const QRect& rect,
                                const QModelIndex& index) const
{
    RepoItem *item = getRepoItem(index);
    if (!item) {
        QTreeView::drawBranches(painter, rect, index);
        return;
    }

    painter->save();
    painter->setFont(awesome->font(kSyncStatusIconSize));
    QString icon_text;
    icon_text.append(QChar(icon_long_arrow_up));
    icon_text.append(QChar(icon_long_arrow_down));
    painter->drawText(rect,
                      Qt::AlignCenter | Qt::AlignVCenter,
                      icon_text);
    painter->restore();
}

RepoItem* RepoTreeView::getRepoItem(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return NULL;
    }
    const RepoTreeModel *model = (const RepoTreeModel*)index.model();
    QStandardItem *item = model->itemFromIndex(index);
    if (item->type() != REPO_ITEM_TYPE) {
        // qDebug("drawBranches: %s\n", item->data(Qt::DisplayRole).toString().toUtf8().data());
        return NULL;
    }
    return (RepoItem *)item;
}
