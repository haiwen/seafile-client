#ifndef SEAFILE_CLIENT_REPO_SERVICE_HELPER_H_
#define SEAFILE_CLIENT_REPO_SERVICE_HELPER_H_

#include <QObject>
#include <QString>
#include <QList>
#include <QScopedPointer>

#include "account.h"
#include "api/api-error.h"
#include "api/server-repo.h"
#include "filebrowser/seaf-dirent.h"
#include "filebrowser/data-mgr.h"

class GetDirentsRequest;
class QWidget;

class FileDownloadHelper : public QObject {
    Q_OBJECT
public:
    static void openFile(const QString& path, bool work_around_mac_auto_udpate);
    FileDownloadHelper(const Account &account, const ServerRepo &repo, const QString &path, QWidget *parent);
    ~FileDownloadHelper();
    void start();

private slots:

    void onCancel();
    void onGetDirentsSuccess(bool current_readonly, const QList<SeafDirent> &dirents);
    void onGetDirentsFailure(const ApiError &)
    {
        downloadFile(QString());
    }

private:
    void downloadFile(const QString &id);

    const Account account_;
    const ServerRepo repo_;
    const QString path_;
    const QString file_name_;
    QWidget *parent_;
    GetDirentsRequest *req_;
};

#endif // SEAFILE_CLIENT_REPO_SERVICE_HELPER_H_
