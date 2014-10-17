#ifndef SEAFILE_CLIENT_FILE_BROWSER_NETWORK_MONO_H
#define SEAFILE_CLIENT_FILE_BROWSER_NETWORK_MONO_H

#include <QObject>
#include <QSet>
class FileNetworkManager;

// A singleton class designed to replace FileNetworkManager outside FileBrowser
// collect all information for all FileNetworkManagers emit
class FileNetworkMono : public QObject {
    Q_OBJECT
public:
    static FileNetworkMono *getInstance() {
        if (!instance_) {
            static FileNetworkMono mono;
            instance_ = &mono;
        }
        return instance_;
    }

    void RegisterManager(FileNetworkManager *mgr) { mgrs_.insert(mgr); }
    void UnregisterManager(FileNetworkManager *mgr) { mgrs_.remove(mgr); }

    qint64 getUploadRate();
    qint64 getDownloadRate();
    qint64 getRate() { return getUploadRate() + getDownloadRate(); }

private:
    FileNetworkMono();
    ~FileNetworkMono() { mgrs_.clear(); }

    QSet<FileNetworkManager *> mgrs_;
    static FileNetworkMono *instance_;


    /* last tranferred bytes */
    qint64 last_time_;
    qint64 upload_last_bytes_;
    qint64 download_last_bytes_;
    qint64 upload_rates_;
    qint64 download_rates_;
};

#endif // SEAFILE_CLIENT_FILE_BROWSER_NETWORK_MONO_H
