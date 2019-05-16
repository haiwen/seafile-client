#ifndef SEAFILE_CLIENT_STARRED_FILES_LIST_MODEL_H
#define SEAFILE_CLIENT_STARRED_FILES_LIST_MODEL_H

#include <vector>
#include <QStandardItemModel>

#include "api/starred-file.h"

class QModelIndex;


class StarredFilesListModel : public QStandardItemModel {
    Q_OBJECT

public:
    StarredFilesListModel(QObject *parent=0);

    void setFiles(const std::vector<StarredItem>& files);

private:

    std::vector<StarredItem> files_;
};

#endif // SEAFILE_CLIENT_STARRED_FILES_LIST_MODEL_H
