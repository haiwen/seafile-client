#ifndef SEAFILE_CLIENT_FILE_TABLE_MODEL_H
#define SEAFILE_CLIENT_FILE_TABLE_MODEL_H

#include <QStack>
#include <QStyledItemDelegate>
#include <QModelIndex>

#include "api/server-repo.h"
#include "account-mgr.h"
#include "seaf-dirent.h"

class ApiError;
class DataManager;
class FileNetworkManager;
class GetDirentsRequest;
class FileNetworkTask;

class FileTableModel : public QAbstractTableModel
{
    Q_OBJECT
signals:
    void loading();
    void loadingFinished();
    void loadingFailed();
    void backwardEnabled(bool enabled);
    void forwardEnabled(bool enabled);
    void downloadEnabled(bool enabled);
    void taskStarted(const FileNetworkTask *task);

public:
    FileTableModel(const ServerRepo& repo, QObject *parent=0);
    ~FileTableModel();

    int rowCount(const QModelIndex& parent=QModelIndex()) const;
    int columnCount(const QModelIndex& parent=QModelIndex()) const;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    const QList<SeafDirent>& dirents();
    void setDirents(const QList<SeafDirent>& dirents);
    const SeafDirent direntAt(int index) const;
    const SeafDirent *selectedDirent() const;

    void setMouseOver(const int row);
    Qt::DropActions supportedDropActions() const;

    const QString& currentPath() { return current_path_; }
    const Account& account() { return account_; }
    const ServerRepo& repo() { return repo_; }
    const FileNetworkManager* fileNetworkManager() { return file_network_mgr_; }

public slots:
    void onSelectionChanged(const int row);
    void onResizeEvent(const QSize &new_size);

    void onEnter(const SeafDirent& dirent);
    void onBackward();
    void onForward();
    void onNavigateHome();
    void onRefresh(bool forcely = false);
    void onRefreshForcely() { onRefresh(true); }

    void onDirEnter(const SeafDirent& dirent);
    void onFileUpload(bool prompt = true);
    void onFileUpload(const QString &file_location);
    void onFileDownload();
    void onFileDownload(const SeafDirent& dirent);
    void onOpenCacheDir();

private slots:
    void onGetDirentsSuccess(const QList<SeafDirent>& dirents);
    void onGetDirentsFailed(const ApiError& error);

private:
    Q_DISABLE_COPY(FileTableModel)

    QList<SeafDirent> dirents_;
    const SeafDirent *selected_dirent_;

    int curr_hovered_;
    int file_name_column_width_;

    DataManager *data_mgr_;
    FileNetworkManager *file_network_mgr_;

    const Account account_;
    const ServerRepo repo_;

    QString current_path_;
    QStack<QString> forward_history_;
    QStack<QString> backward_history_;
};

enum {
    FILE_COLUMN_ICON = 0,
    FILE_COLUMN_NAME,
    FILE_COLUMN_MTIME,
    FILE_COLUMN_SIZE,
    FILE_COLUMN_KIND,
    FILE_MAX_COLUMN
};

#endif  // SEAFILE_CLIENT_FILE_TABLE_MODEL_H
