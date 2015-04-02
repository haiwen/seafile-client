#ifndef SEAFILE_CLIENT_FINDER_SYNC_HOST_H_
#define SEAFILE_CLIENT_FINDER_SYNC_HOST_H_
#include <QObject>
#include <QString>
#include <cstdint>

const int kWatchDirMax = 100;
const int kPathMaxSize = 1024;

struct watch_dir_t {
    char body[kPathMaxSize];
    uint32_t status;
};

class QTimer;
class FinderSyncHost : public QObject {
    Q_OBJECT
public:
    FinderSyncHost();
    ~FinderSyncHost();
    // called from another thread
    size_t getWatchSet(watch_dir_t *front, size_t max_size);
private slots:
    void updateWatchSet();
    void doShareLink(const QString &path);
    void onShareLinkGenerated(const QString& link);
private:
    QTimer *timer_;
};

#endif // SEAFILE_CLIENT_FINDER_SYNC_HOST_H_
