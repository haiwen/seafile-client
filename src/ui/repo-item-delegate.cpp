#include <QPainter>
#include <QApplication>
#include <QPixmap>
#include <QToolTip>
#include <QSortFilterProxyModel>

#include "utils/utils.h"
#include "utils/paint-utils.h"
#include "seafile-applet.h"
#include "settings-mgr.h"
#include "api/server-repo.h"
#include "repo-item.h"
#include "repo-tree-view.h"
#include "repo-tree-model.h"
#include "rpc/rpc-client.h"
#include "rpc/local-repo.h"
#include "account-mgr.h"
#include "repo-service.h"
#include "QtAwesome.h"

#include "repo-item-delegate.h"

namespace {

/**
            RepoName
   RepoIcon          statusIcon
            subtitle
 */

const int kMarginLeft = 16;
const int kMarginRight = 5;
const int kMarginTop = 5;
const int kMarginBottom = 5;
const int kPadding = 5;
const int kRepoItemVerticalMargin = 13;
const int kRepoCategoryVerticalMargin = 10;

const int kRepoIconHeight = 36;
const int kRepoIconWidth = 36;
const int kRepoIconHeightAlpha = 24;
const int kRepoIconWidthAlpha = 24;
const int kRepoNameWidth = 175;
const int kRepoNameHeight = 30;
const int kRepoNameHeightAlpha = 20;
const int kRepoDescriptionHeightAlpha = 14;
const int kRepoStatusIconWidth = 24;
const int kRepoStatusIconHeight = 24;
const int kRepoStatusIconWidthAlpha = 16;
const int kRepoStatusIconHeightAlpha = 16;

const char *kRepoCategoryIndicatorColor = "#BBB";
const int kRepoCategoryNameMaxWidth = 400;
const int kRepoCategoryIndicatorWidth = 16;
const int kRepoCategoryIndicatorHeight = 16;
const int kMarginBetweenIndicatorAndName = 5;

const int kMarginBetweenRepoIconAndName = 10;
const int kMarginBetweenRepoNameAndStatus = 20;

const char *kRepoNameColor = "#141414";
const char *kRepoNameColorHighlighted = "#544D49";
const char *kTimestampColor = "#AAAAAA";
const char *kTimestampColorHighlighted = "#9D9B9A";
const int kRepoNameFontSize = 14;
const int kTimestampFontSize = 12;
const int kRepoDescriptionFontSizeAlpha = 10;
const int kRepoCategoryNameFontSize = 14;
const int kRepoCategoryCountFontSize = 12;
const int kRepoCategoryCountFontSizeAlpha = 14;
const int kOwnerFontSize = 12;
const int kOwnerFontSizeAlpha = 10;

const char *kRepoItemBackgroundColorHighlighted = "#F9E0C7";
const char *kRepoItemBackgroundColorHighlightedAlpha = "#EFEEEE";
const char *kRepoItemBackgroundColor = "white";
const char *kRepoItemBackgroundColorAlpha = "#F9F9F9";
const char *kRepoItemBackgroundColorDragMove = "#C8C8C8";

const char *kRepoCategoryColor = "#747474";
//const char *kRepoCategoryColorHighlighted = "#FAF5FB";

const char *kRepoCategoryBackgroundColor = "white";
const char *kRepoCategoryBackgroundColorAlpha = "#F9F9F9";
//const char *kRepoCategoryBackgroundColorHighlighted = "#EF7544";

const int kRepoCategoryCountMarginRight = 10;
const int kRepoCategoryCountMarginRightAlpha = 20;
const char *kRepoCategoryCountColor = "#AAAAAA";

static void showTooltip(const QString &text,
                        QWidget *viewport,
                        const QRect &rect,
                        const QRect &small_rect)
{
    QPoint tool_tip_pos =
        viewport->mapToGlobal(small_rect.center() + rect.topLeft());
    QToolTip::showText(
        tool_tip_pos, text, viewport, small_rect.translated(rect.topLeft()));
}

} // namespace

RepoItemDelegate::RepoItemDelegate(QObject *parent)
  : QStyledItemDelegate(parent)
{
}

QSize RepoItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                                 const QModelIndex &index) const
{
    QStandardItem *item = getItem(index);
    if (!item) {
        return QStyledItemDelegate::sizeHint(option, index);
    }

    if (item->type() == REPO_ITEM_TYPE) {
        return sizeHintForRepoItem(option, (const RepoItem *)item);
    } else {
        // return QStyledItemDelegate::sizeHint(option, index);
        return sizeHintForRepoCategoryItem(option, (const RepoCategoryItem *)item);
    }
}

QSize RepoItemDelegate::sizeHintForRepoItem(const QStyleOptionViewItem &option,
                                            const RepoItem *item) const
{

    int width = kMarginLeft + kRepoIconWidthAlpha
        + kMarginBetweenRepoIconAndName + kRepoNameWidth
        + kMarginBetweenRepoNameAndStatus + kRepoStatusIconWidthAlpha
        + kMarginRight + kPadding * 2;

    // int height = kRepoIconHeightAlpha + kPadding * 2 + kMarginTop + kMarginBottom;
    int height = kRepoIconHeightAlpha + kRepoItemVerticalMargin * 2;

    // qDebug("width = %d, height = %d\n", width, height);

    return QSize(width, height);
}

QSize RepoItemDelegate::sizeHintForRepoCategoryItem(const QStyleOptionViewItem &option,
                                                    const RepoCategoryItem *item) const
{
    int width, height;

    QFontMetrics qfm(changeFontSize(option.font, kRepoCategoryNameFontSize));
    QSize size = qfm.size(0, item->name());

    width = qMin(size.width(), kRepoCategoryIndicatorWidth)
        + kMarginBetweenIndicatorAndName + kRepoCategoryNameMaxWidth;

    // height = qMax(size.height(), kRepoCategoryIndicatorHeight) + kPadding;
    height = qMax(size.height(), kRepoCategoryIndicatorHeight) + kRepoCategoryVerticalMargin * 2;

    // qDebug("width = %d, height = %d\n", width, height);

    return QSize(width, height);
}


void RepoItemDelegate::paint(QPainter *painter,
                             const QStyleOptionViewItem& option,
                             const QModelIndex& index) const
{
    QStandardItem *item = getItem(index);
    if (!item) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    if (item->type() == REPO_ITEM_TYPE) {
        paintRepoItem(painter, option, (RepoItem *)item);
    } else {
        paintRepoCategoryItem(painter, option, (RepoCategoryItem *)item);
    }

    // fix for showing first category if hidden
    RepoTreeView *view = static_cast<RepoTreeView*>(parent());
    if (view->indexAt(QPoint(0, 0)).parent().isValid()) {
        if (option.rect.contains(QPoint(0, 0))) {
            paintRepoCategoryItem(painter, option, (RepoCategoryItem *)getItem(view->indexAt(QPoint(0, 0)).parent()));
        }
    }
}

void RepoItemDelegate::paintRepoItem(QPainter *painter,
                                     const QStyleOptionViewItem& option,
                                     const RepoItem *item) const
{
    const ServerRepo& repo = item->repo();
    QBrush backBrush;
    bool selected = false;

    RepoTreeModel *model = (RepoTreeModel *)item->model();
    RepoTreeView *view = model->treeView();
    QModelIndex index = ((QSortFilterProxyModel *)view->model())->mapFromSource(model->indexFromItem(item));
    if (option.state & (QStyle::State_HasFocus | QStyle::State_Selected)) {
        backBrush = QColor(kRepoItemBackgroundColorHighlightedAlpha);
        selected = true;
    } else if (view->getCurrentDropTarget() == index) {
        backBrush = QColor(kRepoItemBackgroundColorDragMove);
    } else {
        backBrush = QColor(kRepoItemBackgroundColor);
    }

    painter->save();
    painter->fillRect(option.rect, backBrush);
    painter->restore();

    int indent_left = 0;
    if (item->level() > 0) {
        indent_left = kRepoCategoryIndicatorWidth + kMarginBetweenIndicatorAndName;
    }

    // Paint repo icon
    // QPoint repo_icon_pos(kMarginLeft + kPadding + indent_left, kMarginTop + kPadding);
    QPoint repo_icon_pos(kMarginLeft, kRepoItemVerticalMargin);
    repo_icon_pos += option.rect.topLeft();
    painter->save();

    // get the device pixel radio from current painter device
    double scale_factor = 1;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    scale_factor = globalDevicePixelRatio();
#endif // QT5
    QPixmap repo_icon(repo.getIcon().pixmap(QSize(kRepoIconWidthAlpha, kRepoIconHeightAlpha) * scale_factor));
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    repo_icon.setDevicePixelRatio(scale_factor);
#endif // QT5

    QRect repo_icon_rect(repo_icon_pos, QSize(kRepoIconWidthAlpha, kRepoIconHeightAlpha));
    painter->drawPixmap(repo_icon_rect, repo_icon);
    painter->restore();

    // Paint repo name
    painter->save();
    // QPoint repo_name_pos = repo_icon_pos + QPoint(kRepoIconWidthAlpha + kMarginBetweenRepoIconAndName, 0);
    QPoint repo_name_pos = repo_icon_pos + QPoint(kRepoIconWidthAlpha + kMarginBetweenRepoIconAndName, -5);
    int repo_name_width = option.rect.width() - kRepoIconWidthAlpha - kMarginBetweenRepoIconAndName
        - kRepoStatusIconWidthAlpha - kMarginBetweenRepoNameAndStatus
        - kPadding * 2 - kMarginLeft - kMarginRight;
    repo_name_width -= indent_left;
    int repo_name_height = ::textHeightInFont(repo.name, changeFontSize(painter->font(), kRepoNameFontSize));
    QRect repo_name_rect(repo_name_pos, QSize(repo_name_width, repo_name_height));
    painter->setPen(QColor(selected ? kRepoNameColorHighlighted : kRepoNameColor));
    painter->setFont(changeFontSize(painter->font(), kRepoNameFontSize));
    painter->drawText(repo_name_rect,
                      Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
                      fitTextToWidth(repo.name, painter->font(), repo_name_width),
                      &repo_name_rect);
    painter->restore();

    // Paint repo description
    QString description;
    const LocalRepo& r = item->localRepo();
    if (r.isValid()) {
        if (r.sync_state == LocalRepo::SYNC_STATE_ING) {
            description = r.sync_state_str;
            if (r.has_data_transfer) {
                int rate, percent;
                if (seafApplet->rpcClient()->getRepoTransferInfo(r.id, &rate, &percent) == 0) {
                    description += ", " + QString::number(percent) + "%";
                }
            }
        } else if (r.sync_state == LocalRepo::SYNC_STATE_ERROR) {
            description = r.getErrorString();
        }
    } else {
        const CloneTask& task = item->cloneTask();
        if (task.isValid() && task.isDisplayable()) {
            if (task.error_str.length() > 0) {
                description = task.error_str;
            } else {
                description = task.state_str;
            }
        }
    }

    if (description.isEmpty()) {
        description = translateCommitTime(repo.mtime);
    }
    painter->save();
    QPoint repo_desc_pos = repo_name_rect.bottomLeft() + QPoint(0, 5);
    int repo_desc_height = ::textHeightInFont(description, changeFontSize(painter->font(), kRepoDescriptionFontSizeAlpha));
    QRect repo_desc_rect(repo_desc_pos, QSize(repo_name_width, repo_desc_height));
    painter->setFont(changeFontSize(painter->font(), kRepoDescriptionFontSizeAlpha));
    painter->setPen(QColor(selected ? kTimestampColorHighlighted : kTimestampColor));
    painter->drawText(repo_desc_rect,
                      Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
                      fitTextToWidth(description, painter->font(), repo_name_width),
                      &repo_desc_rect);
    painter->restore();

    // Paint extra description
    QString extra_description;
    if (repo.isSubfolder()) {
        ServerRepo parent_repo = RepoService::instance()->getRepo(repo.parent_repo_id);
        extra_description = tr(", %1%2").arg(parent_repo.name).arg(repo.parent_path);
    }
    // Paint repo sharing owner for private share
    if (static_cast<RepoCategoryItem*>(item->parent())->categoryIndex() ==
        RepoTreeModel::CAT_INDEX_SHARED_REPOS)
        extra_description += tr(", %1").arg(repo.owner.split('@').front());

    if (!extra_description.isEmpty()) {
        painter->save();

        int width = option.rect.topRight().x() - 40 - repo_desc_rect.topRight().x();
        if (width < 3)
          width = 3;
        QPoint repo_owner_pos = repo_desc_rect.topRight();
        int extra_desc_height = ::textHeightInFont(extra_description, changeFontSize(painter->font(), kOwnerFontSizeAlpha));
        QRect repo_owner_rect(repo_owner_pos, QSize(width, extra_desc_height));
        painter->setFont(changeFontSize(painter->font(), kOwnerFontSizeAlpha));
        painter->setPen(QColor(selected ? kTimestampColorHighlighted : kTimestampColor));
        painter->drawText(repo_owner_rect,
                          Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
                          fitTextToWidth(extra_description, painter->font(), width),
                          &repo_owner_rect);

        painter->restore();
    }

    // Paint repo status icon
    QPoint status_icon_pos = option.rect.topRight() - QPoint(41, 0);
    status_icon_pos.setY(option.rect.center().y() - (kRepoStatusIconHeightAlpha / 2));
    QRect status_icon_rect(status_icon_pos, QSize(kRepoStatusIconWidthAlpha, kRepoStatusIconHeightAlpha));

    QPixmap status_icon_pixmap = getSyncStatusIcon(item).pixmap(scale_factor * status_icon_rect.size());
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    status_icon_pixmap.setDevicePixelRatio(scale_factor);
#endif // QT5

    painter->save();
    painter->drawPixmap(status_icon_rect, status_icon_pixmap);
    painter->restore();

    // Update the metrics of this item
    RepoItem::Metrics metrics;
    QPoint shift(-option.rect.topLeft().x(), -option.rect.topLeft().y());
    metrics.icon_rect = repo_icon_rect;
    metrics.name_rect = repo_name_rect;
    metrics.subtitle_rect = repo_desc_rect;
    metrics.status_icon_rect = status_icon_rect;

    metrics.icon_rect.translate(shift);
    metrics.name_rect.translate(shift);
    metrics.subtitle_rect.translate(shift);
    metrics.status_icon_rect.translate(shift);

    item->setMetrics(metrics);
}

void RepoItemDelegate::paintRepoCategoryItem(QPainter *painter,
                                             const QStyleOptionViewItem& option,
                                             const RepoCategoryItem *item) const
{
    QBrush backBrush = QColor(kRepoCategoryBackgroundColor);

    painter->save();
    painter->fillRect(option.rect, backBrush);
    painter->restore();

    // Paint the expand/collapse indicator
    painter->save();

    RepoTreeModel *model = (RepoTreeModel *)item->model();
    RepoTreeView *view = model->treeView();
    QModelIndex index = ((QSortFilterProxyModel *)view->model())->mapFromSource(model->indexFromItem(item));
    bool expanded = view->isExpanded(index);
    int indent_left = 0;
    if (item->level() > 0) {
        indent_left = kRepoCategoryIndicatorWidth + kMarginBetweenIndicatorAndName;
    }

    int repo_category_text_height = ::textHeightInFont(item->name(), changeFontSize(option.font, kRepoCategoryNameFontSize));
    int vertical_padding_between_indicator_and_name = (repo_category_text_height - kRepoCategoryIndicatorHeight) / 2;

    QRect indicator_rect(option.rect.topLeft() +
                         QPoint(kMarginLeft + indent_left,
                                kRepoCategoryVerticalMargin + vertical_padding_between_indicator_and_name),
                         QSize(kRepoCategoryIndicatorWidth, kRepoCategoryIndicatorHeight));
    // get the device pixel radio from current painter device
    double scale_factor = 1;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    scale_factor = globalDevicePixelRatio();
#endif // QT5
    QIcon icon(expanded ? awesome->icon(icon_caret_down, kRepoCategoryIndicatorColor)
                        : awesome->icon(icon_caret_right, kRepoCategoryIndicatorColor));
    QPixmap icon_pixmap(icon.pixmap(QSize(kRepoCategoryIndicatorWidth, kRepoCategoryIndicatorWidth) * scale_factor));
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    icon_pixmap.setDevicePixelRatio(scale_factor);
#endif // QT5
    painter->drawPixmap(indicator_rect, icon_pixmap);
    painter->restore();

    // Paint category name

    // calculate the count of synced repos
    int synced_repos = 0;
    for (int i = 0; i < item->rowCount(); ++i) {
        QStandardItem *child = item->child(i);
        if (child->type() == REPO_ITEM_TYPE) {
            RepoItem *repo_item = static_cast<RepoItem *>(child);
            if (repo_item->localRepo().isValid())
                ++synced_repos;
        } else {
            RepoCategoryItem *cat_item = static_cast<RepoCategoryItem *>(child);
            for (int j = 0; j < cat_item->rowCount(); ++j) {
                RepoItem *repo_item = static_cast<RepoItem *>(cat_item->child(j));
                if (repo_item->localRepo().isValid())
                    ++synced_repos;
            }
        }
    }
    const QString category_count_text = QString::number(synced_repos) + "/" + QString::number(item->matchedReposCount());
    const int category_count_width = ::textWidthInFont(category_count_text, changeFontSize(option.font, kRepoCategoryCountFontSizeAlpha));

    painter->save();
    QPoint category_name_pos = indicator_rect.topRight() +
                               QPoint(kMarginBetweenIndicatorAndName, - vertical_padding_between_indicator_and_name);
    QRect category_name_rect(category_name_pos,
                             option.rect.bottomRight() - QPoint(kPadding + category_count_width + kRepoCategoryCountMarginRightAlpha,
                                                                kRepoCategoryVerticalMargin));
    painter->setPen(QColor(kRepoCategoryColor));
    painter->setFont(changeFontSize(option.font, kRepoCategoryNameFontSize));
    painter->drawText(category_name_rect,
                      Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
                      fitTextToWidth(item->name(), painter->font(), category_name_rect.width()));
    painter->restore();

    // Paint category count
    painter->save();
    QPoint category_count_pos = option.rect.topRight() +
                                QPoint(-category_count_width - kRepoCategoryCountMarginRightAlpha, 0);
    category_count_pos.setY(category_name_pos.y());
    QRect category_count_rect(category_count_pos,
                              option.rect.bottomRight() - QPoint(kRepoCategoryCountMarginRightAlpha, kRepoCategoryVerticalMargin));
    painter->setFont(changeFontSize(option.font, kRepoCategoryCountFontSizeAlpha));
    painter->setPen(QColor(kRepoCategoryCountColor));
    painter->drawText(category_count_rect,
                      Qt::AlignLeft | Qt::AlignTop,
                      category_count_text);
    painter->restore();
}

QIcon RepoItemDelegate::getSyncStatusIcon(const RepoItem *item) const
{
    const QString prefix = ":/images/sync/";
    const LocalRepo& repo = item->localRepo();
    QString icon;
    if (!repo.isValid()) {
        icon = "cloudUnsynced-alpha";
    } else if (!seafApplet->settingsManager()->autoSync()) {
        icon = "pause";
    } else {
        switch (repo.sync_state) {
        case LocalRepo::SYNC_STATE_DONE:
            icon = "cloudSynced-alpha";
            break;
        case LocalRepo::SYNC_STATE_ING:
            icon = "cloudSync-alpha";
            break;
        case LocalRepo::SYNC_STATE_ERROR:
            icon = "exclamation";
            break;
        case LocalRepo::SYNC_STATE_WAITING:
            icon = "waiting";
            break;
        case LocalRepo::SYNC_STATE_DISABLED:
            icon = "pause";
            break;
        case LocalRepo::SYNC_STATE_UNKNOWN:
            icon = "question";
            break;
        case LocalRepo::SYNC_STATE_INIT:
            // If the repo is in "sync init", we just display the previous
            // icon.
            icon = last_icon_map_.value(repo.id, "waiting");
            break;
        }
    }

    last_icon_map_[repo.id] = icon;

    return QIcon(prefix + icon + ".png");
}

QStandardItem* RepoItemDelegate::getItem(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return NULL;
    }
    QSortFilterProxyModel *proxy = (QSortFilterProxyModel *)index.model();
    RepoTreeModel *tree_model_ = (RepoTreeModel *)(proxy->sourceModel());
    QStandardItem *item = tree_model_->itemFromIndex(proxy->mapToSource(index));
    if (item->type() != REPO_ITEM_TYPE &&
        item->type() != REPO_CATEGORY_TYPE) {
        return NULL;
    }
    return item;
}

void RepoItemDelegate::showRepoItemToolTip(const RepoItem *item,
                                           const QPoint& global_pos,
                                           QWidget *viewport,
                                           const QRect& rect) const
{
    const RepoItem::Metrics& metrics = item->metrics();

    const QRect& status_icon_rect = metrics.status_icon_rect;
    const QRect& subtitle_rect = metrics.subtitle_rect;

    QPoint viewpos = viewport->mapFromGlobal(global_pos);
    viewpos -= rect.topLeft();

    if (status_icon_rect.contains(viewpos)) {
        QString text = "<p style='white-space:pre'>";
        const LocalRepo& local_repo = item->localRepo();
        if (!local_repo.isValid()) {
            text += tr("This library has not been downloaded");
        } else {
            if (local_repo.sync_state == LocalRepo::SYNC_STATE_ERROR) {
                text += local_repo.getErrorString();
            } else {
                text += local_repo.sync_state_str;
            }
        }
        text += "</p>";

        showTooltip(text, viewport, rect, status_icon_rect);

    } else if (subtitle_rect.contains(viewpos)) {
        QString text = "<p style='white-space:pre'>";
        const LocalRepo& local_repo = item->localRepo();
        if (local_repo.isValid() && local_repo.sync_state == LocalRepo::SYNC_STATE_ERROR) {
            text += local_repo.getErrorString();
            text += "</p>";
            showTooltip(text, viewport, rect, subtitle_rect);
        }
    }
}
