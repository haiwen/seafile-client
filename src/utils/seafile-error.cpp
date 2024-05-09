#include "seafile-error.h"
#if defined(_MSC_VER)
#include "include/seafile-error.h"
#else
#include <seafile/seafile-error.h>
#endif

QString translateSyncErrorCode(const int error_code)
{
    QString error_str;
    switch (error_code) {
        case SYNC_ERROR_ID_FILE_LOCKED_BY_APP:
            error_str = QObject::tr("File is locked by another application");
            break;
        case SYNC_ERROR_ID_FOLDER_LOCKED_BY_APP:
            error_str = QObject::tr("Folder is locked by another application");
            break;
        case SYNC_ERROR_ID_FILE_LOCKED:
            error_str = QObject::tr("File is locked by another user");
            break;
        case SYNC_ERROR_ID_INVALID_PATH:
            error_str = QObject::tr("Path is invalid");
            break;
        case SYNC_ERROR_ID_INDEX_ERROR:
            error_str = QObject::tr("Error when indexing");
            break;
        case SYNC_ERROR_ID_PATH_END_SPACE_PERIOD:
            error_str = QObject::tr("Path ends with space or period character");
            break;
        case SYNC_ERROR_ID_PATH_INVALID_CHARACTER:
            error_str = QObject::tr("Path contains invalid characters like '|' or ':'");
            break;
        case SYNC_ERROR_ID_FOLDER_PERM_DENIED:
            error_str = QObject::tr("Update to file denied by folder permission setting");
            break;
        case SYNC_ERROR_ID_PERM_NOT_SYNCABLE:
            error_str = QObject::tr("Syncing is denied by cloud-only permission settings");
            break;
        case SYNC_ERROR_ID_UPDATE_TO_READ_ONLY_REPO:
            error_str = QObject::tr("Created or updated a file in a non-writable library or folder");
            break;
        case SYNC_ERROR_ID_ACCESS_DENIED:
            error_str = QObject::tr("Permission denied on server");
            break;
        case SYNC_ERROR_ID_NO_WRITE_PERMISSION:
            error_str = QObject::tr("Do not have write permission to the library");
            break;
        case SYNC_ERROR_ID_QUOTA_FULL:
            error_str = QObject::tr("Storage quota full");
            break;
        case SYNC_ERROR_ID_NETWORK:
            error_str = QObject::tr("Network error");
            break;
        case SYNC_ERROR_ID_RESOLVE_PROXY:
            error_str = QObject::tr("Cannot resolve proxy address");
            break;
        case SYNC_ERROR_ID_RESOLVE_HOST:
            error_str = QObject::tr("Cannot resolve server address");
            break;
        case SYNC_ERROR_ID_CONNECT:
            error_str = QObject::tr("Cannot connect to server");
            break;
        case SYNC_ERROR_ID_SSL:
            error_str = QObject::tr("Failed to establish secure connection. Please check server SSL certificate");
            break;
        case SYNC_ERROR_ID_TX:
            error_str = QObject::tr("Data transfer was interrupted. Please check network or firewall");
            break;
        case SYNC_ERROR_ID_TX_TIMEOUT:
            error_str = QObject::tr("Data transfer timed out. Please check network or firewall");
            break;
        case SYNC_ERROR_ID_UNHANDLED_REDIRECT:
            error_str = QObject::tr("Unhandled http redirect from server. Please check server cofiguration");
            break;
        case SYNC_ERROR_ID_SERVER:
            error_str = QObject::tr("Server error");
            break;
        case SYNC_ERROR_ID_LOCAL_DATA_CORRUPT:
            error_str = QObject::tr("Internal data corrupt on the client. Please try to resync the library");
            break;
        case SYNC_ERROR_ID_WRITE_LOCAL_DATA:
            error_str = QObject::tr("Failed to write data on the client. Please check disk space or folder permissions");
            break;
        case SYNC_ERROR_ID_SERVER_REPO_DELETED:
            error_str = QObject::tr("Library deleted on server");
            break;
        case SYNC_ERROR_ID_SERVER_REPO_CORRUPT:
            error_str = QObject::tr("Library damaged on server");
            break;
        case SYNC_ERROR_ID_NOT_ENOUGH_MEMORY:
            error_str = QObject::tr("Not enough memory");
            break;
        case SYNC_ERROR_ID_CONFLICT:
            error_str = QObject::tr("Concurrent updates to file. File is saved as conflict file");
            break;
        case SYNC_ERROR_ID_GENERAL_ERROR:
            error_str = QObject::tr("Unknown error");
            break;
        case SYNC_ERROR_ID_REMOVE_UNCOMMITTED_FOLDER:
            error_str = QObject::tr("A folder that may contain not-yet-uploaded files is moved to seafile-recycle-bin folder.");
            break;
#if !defined(Q_OS_WIN32)
        case SYNC_ERROR_ID_INVALID_PATH_ON_WINDOWS:
            error_str = QObject::tr("The file path contains symbols that are not supported by the Windows system");
            break;
#endif
        case SYNC_ERROR_ID_LIBRARY_TOO_LARGE:
            error_str = QObject::tr("Library contains too many files.");
            break;
        case SYNC_ERROR_ID_DEL_CONFIRMATION_PENDING:
            error_str = QObject::tr("Waiting for confirmation to delete files");
            break;
        case SYNC_ERROR_ID_TOO_MANY_FILES:
            error_str = QObject::tr("Too many files in library");
            break;
        case SYNC_ERROR_ID_CHECKOUT_FILE:
            error_str = QObject::tr("Failed to download file. Please check disk space or folder permissions");
            break;
        case SYNC_ERROR_ID_BLOCK_MISSING:
            error_str = QObject::tr("Failed to upload file blocks. Please check network or firewall");
            break;
        case SYNC_ERROR_ID_CASE_CONFLICT:
            error_str = QObject::tr("Path has character case conflict with existing file or folder. Will not be downloaded");
            break;
        default:
            qWarning("Unknown sync error");
    }
    return error_str;
}
