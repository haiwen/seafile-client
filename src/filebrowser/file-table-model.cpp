#include "file-table-model.h"

#include <QtGlobal>
#include <QFileDialog>
#include <QMessageBox>
#include <QDesktopServices>

#include "seaf-dirent.h"
#include "seafile-applet.h"
#include "api/api-error.h"
#include "utils/utils.h"
#include "utils/file-utils.h"

#include "data-mgr.h"
#include "file-network-mgr.h"
#include "network/task.h"

const int kDefaultColumnWidth = 120;
const int kDefaultColumnHeight = 40;
const int kColumnIconSize = 36;
const int kColumnIconAlign = 4;
const int kDefaultColumnSum = kDefaultColumnWidth * 3 + kColumnIconSize + kColumnIconAlign;

FileTableModel::FileTableModel(const ServerRepo& repo, QObject *parent)
    : QAbstractTableModel(parent),
      selected_dirent_(NULL),
      curr_hovered_(-1), // -1 is a publicly-known magic number
      file_name_column_width_(160),
      data_mgr_(NULL),
      file_network_mgr_(NULL),
      account_(seafApplet->accountManager()->currentAccount()),
      repo_(repo),
      current_path_("/")
{
    data_mgr_ = new DataManager(account_, repo_);
    file_network_mgr_ = new FileNetworkManager(account_, repo_.id);

    connect(data_mgr_, SIGNAL(getDirentsSuccess(const QList<SeafDirent>&)),
            this, SLOT(onGetDirentsSuccess(const QList<SeafDirent>&)));

    connect(data_mgr_, SIGNAL(getDirentsFailed(const ApiError&)),
            this, SLOT(onGetDirentsFailed(const ApiError&)));

    connect(file_network_mgr_, SIGNAL(taskStarted(const FileNetworkTask*)),
            this, SIGNAL(taskStarted(const FileNetworkTask*)));
}
FileTableModel::~FileTableModel()
{
    delete data_mgr_;
    delete file_network_mgr_;
}

void FileTableModel::setDirents(const QList<SeafDirent>& dirents)
{
    dirents_ = dirents;
    this->reset();
}

const QList<SeafDirent>& FileTableModel::dirents()
{
    return dirents_;
}

int FileTableModel::rowCount(const QModelIndex& /* parent */) const
{
    return dirents_.size();
}

int FileTableModel::columnCount(const QModelIndex& /* parent */) const
{
    return FILE_MAX_COLUMN;
}

QVariant FileTableModel::data(const QModelIndex & index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    const int column = index.column();
    const int row = index.row();
    const SeafDirent& dirent = dirents_[row];

    if (role == Qt::DecorationRole && column == FILE_COLUMN_ICON) {
        return (dirent.isDir() ?
            QIcon(":/images/folder.png") :
            QIcon(getIconByFileName(dirent.name))).
          pixmap(kColumnIconSize, kColumnIconSize);
    }

    if (role == Qt::TextAlignmentRole && column == FILE_COLUMN_ICON)
        return Qt::AlignRight + Qt::AlignVCenter;

    if (role == Qt::TextAlignmentRole && column == FILE_COLUMN_NAME)
        return Qt::AlignLeft + Qt::AlignVCenter;

    if (role == Qt::BackgroundRole && row == curr_hovered_)
        return QColor(200, 200, 220, 255);

    if (role == Qt::SizeHintRole) {
        QSize qsize(kDefaultColumnWidth, kDefaultColumnHeight);
        switch (column) {
        case FILE_COLUMN_ICON:
          qsize.setWidth(kColumnIconSize + kColumnIconAlign);
          break;
        case FILE_COLUMN_NAME:
          qsize.setWidth(file_name_column_width_);
          break;
        case FILE_COLUMN_SIZE:
          break;
        case FILE_COLUMN_MTIME:
          break;
        case FILE_COLUMN_KIND:
          break;
        default:
          break;
        }
        return qsize;
    }

    //change color only for file name column
    if (role == Qt::ForegroundRole && column == FILE_COLUMN_NAME)
        return QColor("#e83");

    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    switch (column) {
    case FILE_COLUMN_NAME:
      return dirent.name;
    case FILE_COLUMN_SIZE:
      if (dirent.isDir())
        return "";
      return ::readableFileSize(dirent.size);
    case FILE_COLUMN_MTIME:
      return ::translateCommitTime(dirent.mtime);
    case FILE_COLUMN_KIND:
      //TODO: mime file information
      return dirent.isDir() ?
        tr("Folder") :
        tr("Document");
    default:
      return QVariant();
    }
}

QVariant FileTableModel::headerData(int section,
                                    Qt::Orientation orientation,
                                    int role) const
{
    if (orientation == Qt::Vertical)
        return QVariant();

    if (role == Qt::TextAlignmentRole)
        return Qt::AlignLeft + Qt::AlignVCenter;

    if (role != Qt::DisplayRole)
        return QVariant();

    switch (section) {
    case FILE_COLUMN_ICON:
        return "";
    case FILE_COLUMN_NAME:
        return tr("Name");
    case FILE_COLUMN_SIZE:
        return tr("Size");
    case FILE_COLUMN_MTIME:
        return tr("Last Modified");
    case FILE_COLUMN_KIND:
        return tr("Kind");
    default:
        return QVariant();
    }

}

const SeafDirent FileTableModel::direntAt(int index) const
{
    if (index > dirents_.size()) {
        return SeafDirent();
    }

    return dirents_[index];
}

void FileTableModel::setMouseOver(const int row)
{
    const int curr_hovered = curr_hovered_;
    if (curr_hovered_ == row)
        return;
    curr_hovered_ = row;

    if (curr_hovered != -1)
        emit dataChanged(index(curr_hovered, 0),
                         index(curr_hovered, FILE_MAX_COLUMN-1));
    if (curr_hovered_ != -1)
        emit dataChanged(index(curr_hovered_, 0),
                         index(curr_hovered_, FILE_MAX_COLUMN-1));
}

const SeafDirent *FileTableModel::selectedDirent() const
{
    return selected_dirent_;
}

Qt::DropActions FileTableModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}

void FileTableModel::onSelectionChanged(const int row)
{
    if (row != -1) {
        selected_dirent_ = &dirents_[row];
        emit dataChanged(index(row, 0), index(row, FILE_MAX_COLUMN-1));
        // dir download is not implemented
        selected_dirent_->isDir() ? emit downloadEnabled(false) : emit downloadEnabled(true);
    }
    else {
        selected_dirent_ = NULL;
        emit downloadEnabled(false);
    }
    return;
}

void FileTableModel::onResizeEvent(const QSize &new_size)
{
    file_name_column_width_ = new_size.width() - kDefaultColumnSum;
}

void FileTableModel::onEnter(const SeafDirent& dirent)
{
    if (dirent.isDir()) {
        onDirEnter(dirent);
    } else {
        onFileDownload();
    }
}

void FileTableModel::onDirEnter(const SeafDirent& dirent)
{
    backward_history_.push(current_path_);
    emit backwardEnabled(true);

    current_path_ += dirent.name;

    if (!forward_history_.isEmpty()) {
        if (forward_history_.last() == current_path_) {
            forward_history_.pop_back();
            if (forward_history_.isEmpty())
                emit forwardEnabled(false);
        }
        else {
            forward_history_.clear();
            emit forwardEnabled(false);
        }

    }

    onRefresh();
}


void FileTableModel::onBackward()
{
    if (backward_history_.isEmpty())
        return;

    forward_history_.push(current_path_);
    emit forwardEnabled(true);

    current_path_ = backward_history_.pop();
    if (backward_history_.isEmpty())
        emit backwardEnabled(false);

    onRefresh();
}

void FileTableModel::onForward()
{
    if (forward_history_.isEmpty())
        return;

    backward_history_.push(current_path_);
    emit backwardEnabled(true);

    current_path_ = forward_history_.pop();
    if (forward_history_.isEmpty())
        emit forwardEnabled(false);

    onRefresh();
}

void FileTableModel::onNavigateHome()
{
    if (current_path_ == "/")
        return;

    backward_history_.push(current_path_);
    current_path_ = "/";

    if (!forward_history_.isEmpty()) {
        if (forward_history_.last() == current_path_) {
            forward_history_.pop_back();
            if (forward_history_.isEmpty())
                forwardEnabled(false);
        }
        else {
            forward_history_.clear();
            forwardEnabled(false);
        }

    }

    onRefresh();
}

void FileTableModel::onRefresh(bool forcely)
{
    if (!current_path_.endsWith("/"))
        current_path_ += "/";

    emit loading();
    data_mgr_->getDirents(current_path_, forcely);
}

void FileTableModel::onGetDirentsSuccess(const QList<SeafDirent>& dirents)
{
    setDirents(dirents);
    emit loadingFinished();
}

void FileTableModel::onGetDirentsFailed(const ApiError& error)
{
    Q_UNUSED(error);
    emit loadingFailed();
}

void FileTableModel::onFileUpload(bool prompt)
{
    if (prompt) {
        QStringList files = QFileDialog::getOpenFileNames(NULL, tr("Select file to upload"));
        foreach(const QString &file, files) {
            onFileUpload(file);
        }
    }
    else
        onFileUpload("");
}

void FileTableModel::onFileUpload(const QString &_file_name)
{
    QString file_name(_file_name);
    if (file_name.isEmpty())
        return;

    QFileInfo file_info(file_name);
    if (file_info.isDir() || !file_info.exists() || !file_info.isReadable())
        return;

    FileNetworkTask *task = file_network_mgr_->createUploadTask(
          current_path_, file_info.fileName(), file_info.absoluteFilePath());
    connect(task, SIGNAL(finished()), this, SLOT(onRefreshForcely()));
    file_network_mgr_->runTask(task);
}

void FileTableModel::onFileDownload()
{
    if (selected_dirent_)
        onFileDownload(*selected_dirent_);
}

void FileTableModel::onFileDownload(const SeafDirent& dirent)
{
    if (dirent.isDir()) //no implemented yet
        return;
    FileNetworkTask* task =
      file_network_mgr_->createDownloadTask(current_path_,
                               dirent.name, dirent.id);
    file_network_mgr_->runTask(task);
}

void FileTableModel::onOpenCacheDir()
{
    QDir file_cache_dir_(defaultFileCachePath());
    QString file_location(
        file_cache_dir_.absoluteFilePath(repo_.id + current_path_));
    if (file_cache_dir_.mkpath(file_location))
        QDesktopServices::openUrl(QUrl::fromLocalFile(file_location));
    else
        QMessageBox::warning(NULL,
            tr("Warning"),
            tr("Unable to open cache file dir: %1").arg(file_location));

}
