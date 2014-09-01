#ifndef SEAFILE_NETWORK_TASK_BUILDER_H
#define SEAFILE_NETWORK_TASK_BUILDER_H

#include <QString>
class Account;
class SeafileNetworkTask;
class SeafileDownloadTask;
class SeafileUploadTask;

class SeafileNetworkTaskBuilder
{
public:
    SeafileNetworkTaskBuilder() {}
    SeafileDownloadTask* createDownloadTask(const Account &account,
                                           const QString &repo_id,
                                           const QString &path,
                                           const QString &filename,
                                           const QString &file_location);
    SeafileDownloadTask* createDownloadTask(const Account &account,
                                           const QString &repo_id,
                                           const QString &path,
                                           const QString &filename,
                                           const QString &revision,
                                           const QString &file_location);

    SeafileUploadTask* createUploadTask(const Account &account,
                                           const QString &repo_id,
                                           const QString &path,
                                           const QString &filename,
                                           const QString &file_location);
    SeafileUploadTask* createUpdateTask(const Account &account,
                                           const QString &repo_id,
                                           const QString &path,
                                           const QString &filename,
                                           const QString &file_location);
};












#endif // SEAFILE_NETWORK_TASK_BUILDER_H
