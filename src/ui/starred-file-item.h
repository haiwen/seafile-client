#ifndef SEAFILE_CLIENT_STARRED_FILE_ITEM_H
#define SEAFILE_CLIENT_STARRED_FILE_ITEM_H

#include <QStandardItem>
#include "api/starred-file.h"

/**
 * Represent a repo
 */
class StarredFileItem : public QStandardItem {
public:
    explicit StarredFileItem(const StarredFile& repo);

    const StarredFile& file() const { return file_; }

    /**
     * Every time the item is painted, we record the metrics of each part of
     * the item on the screen. So later we the mouse click/hover the item, we
     * can decide which part is hovered, and to do corresponding actions.
     */
    struct Metrics {
        QRect icon_rect;
        QRect name_rect;
        QRect subtitle_rect;
        QRect status_icon_rect;
    };

    void setMetrics(const Metrics& metrics) const { metrics_ = metrics; }
    const Metrics& metrics() const { return metrics_; }

private:
    StarredFile file_;

    mutable Metrics metrics_;
};

#endif // SEAFILE_CLIENT_STARRED_FILE_ITEM_H
