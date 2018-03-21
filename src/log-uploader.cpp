
#include <QtGlobal>

#include <QSysInfo>

#include "utils/utils.h"
#include "utils/file-utils.h"
#include "seafile-applet.h"
#include "configurator.h"
#include "account-mgr.h"

#include "log-uploader.h"
#include "JlCompress.h"

void LogDirUploader::run() {
    QString log_path = QDir(seafApplet->configurator()->ccnetDir()).absoluteFilePath("logs");
    QString username = seafApplet->accountManager()->currentAccount().username;
    QString time = QDateTime::currentDateTime().toString("yyyy.MM.dd-hh.mm.ss");
    compressed_file_name_ = log_path + '-' + time + '-' + username + ".zip";

    if (!JlCompress::compressDir(compressed_file_name_, log_path)) {
        qWarning("create log archive failed");
        emit compressFinished(false);
    } else {
        emit compressFinished(true, compressed_file_name_);
    }
}
