#ifndef SEAFILE_CLIENT_FINDER_SYNC_HOST_H_
#define SEAFILE_CLIENT_FINDER_SYNC_HOST_H_
#include <QObject>
#include <QString>
#include <cstdint>
#include <vector>
#include "utils/stl.h"

const int kWatchDirMax = 100;
const int kPathMaxSize = 1024;

class QTimer;
class FinderSyncHost : public QObject {
    Q_OBJECT
public:
    FinderSyncHost();
    ~FinderSyncHost();
    // called from another thread
    utils::BufferArray getWatchSet(size_t header_size, int max_size = -1);
private slots:
    void updateWatchSet();
    void doShareLink(const QString &path);
    void onShareLinkGenerated(const QString& link);
private:
    QTimer *timer_;
};

#endif // SEAFILE_CLIENT_FINDER_SYNC_HOST_H_
