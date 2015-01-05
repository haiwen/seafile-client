#include <QTimer>
#include <QHash>
#include <QDebug>
#include <algorithm>            // std::sort

#include "utils/utils.h"
#include "seafile-applet.h"
#include "main-window.h"
#include "rpc/rpc-client.h"
#include "starred-file-item.h"

#include "starred-files-list-model.h"

namespace {

bool compareFileByTimestamp(const StarredFile& a, const StarredFile& b)
{
    return a.mtime > b.mtime;
}

} // namespace

StarredFilesListModel::StarredFilesListModel(QObject *parent)
    : QStandardItemModel(parent)
{
}

void StarredFilesListModel::setFiles(const std::vector<StarredFile>& files)
{
    int i, n = files.size();

    clear();

    std::vector<StarredFile> list = files;

    std::sort(list.begin(), list.end(), compareFileByTimestamp);

    for (i = 0; i < n; i++) {
        StarredFileItem *item = new StarredFileItem(files[i]);
        appendRow(item);
    }
}
