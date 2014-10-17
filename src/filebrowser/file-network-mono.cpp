#include "file-network-mono.h"

#include <QDateTime>

#include "file-network-mgr.h"

FileNetworkMono *FileNetworkMono::instance_ = NULL;

namespace {
    static qint64 current_time() { return QDateTime::currentDateTime().toMSecsSinceEpoch(); }
}

FileNetworkMono::FileNetworkMono()
    : last_time_(current_time() - 100), // a hack
      upload_last_bytes_(0), download_last_bytes_(0) {}

qint64 FileNetworkMono::getUploadRate() {
    qint64 upload_bytes = 0;
    foreach(FileNetworkManager* mgr, mgrs_)
    {
        upload_bytes += mgr->getUploadedBytes();
    }

    qint64 ctime = current_time();
    qint64 delta = upload_bytes - upload_last_bytes_;

    if (delta < 0)
        upload_rates_ = 0;
    else if (ctime - last_time_ < 100)
        return upload_rates_;

    upload_rates_ = delta * 1000 / (ctime - last_time_);
    upload_last_bytes_ = upload_bytes;
    last_time_ = ctime;

    return upload_rates_ & (int)-1;
}

qint64 FileNetworkMono::getDownloadRate() {
    qint64 download_bytes = 0;
    foreach(FileNetworkManager* mgr, mgrs_)
    {
        download_bytes += mgr->getDownloadedBytes();
    }

    qint64 ctime = current_time();
    qint64 delta = download_bytes - download_last_bytes_;

    if (delta < 0)
        download_rates_ = 0;
    else if (ctime - last_time_ < 100)
        return download_rates_;

    download_rates_ = delta * 1000 / (ctime - last_time_);
    download_last_bytes_ = download_bytes;
    last_time_ = ctime;

    return download_rates_ & (int)-1;
}

qint64 FileNetworkMono::getRate() {
    return getUploadRate() + getDownloadRate();
}

