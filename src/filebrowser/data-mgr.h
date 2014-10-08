#ifndef SEAFILE_CLIENT_FILE_BROWSER_DATA_MANAGER_H
#define SEAFILE_CLIENT_FILE_BROWSER_DATA_MANAGER_H

#include <QObject>
#include <QList>
#include <QCache>

#include "api/api-error.h"
#include "account.h"
#include "seaf-dirent.h"

class GetDirentsRequest;
template<typename Key, typename T> class LRUCache;

/**
 * DataManager is responsible for getting dirents/files from seahub, as well
 * as the caching policy.
 *
 */
class DataManager : public QObject {
    Q_OBJECT
public:
    DataManager(const Account& account, const ServerRepo& repo);

    ~DataManager();

    void getDirents(const QString& path,
                    bool force_update = false);

signals:
    void getDirentsSuccess(const QList<SeafDirent>& dirents);
    void getDirentsFailed(const ApiError& error);

private slots:
    void onGetDirentsSuccess(const QString &dir_id,
                             const QList<SeafDirent>& dirents);
    void onGetDirentsFailed(const ApiError& error);

private:
    const Account &account_;

    const ServerRepo &repo_;

    QCache<QString, LRUCache<QString, QList<SeafDirent> > > repo_cache_;

    GetDirentsRequest *req_;
};

#endif // SEAFILE_CLIENT_FILE_BROWSER_DATA_MANAGER_H
