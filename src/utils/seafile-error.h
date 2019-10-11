#ifndef SEAFILE_CLIENT_SEAFILE_ERROR_H
#define SEAFILE_CLIENT_SEAFILE_ERROR_H

#define SYNC_ERROR_ID_FILE_LOCKED_BY_APP        0                   // SYNC_ERROR_LEVEL_FILE
#define SYNC_ERROR_ID_FOLDER_LOCKED_BY_APP      1                   // SYNC_ERROR_LEVEL_FILE
#define SYNC_ERROR_ID_FILE_LOCKED               2                   // SYNC_ERROR_LEVEL_FILE
#define SYNC_ERROR_ID_INVALID_PATH              3                   // SYNC_ERROR_LEVEL_FILE
#define SYNC_ERROR_ID_INDEX_ERROR               4                   // SYNC_ERROR_LEVEL_FILE
#define SYNC_ERROR_ID_PATH_END_SPACE_PERIOD     5                   // SYNC_ERROR_LEVEL_FILE
#define SYNC_ERROR_ID_PATH_INVALID_CHARACTER    6                   // SYNC_ERROR_LEVEL_FILE
#define SYNC_ERROR_ID_FOLDER_PERM_DENIED        7                   // SYNC_ERROR_LEVEL_FILE
#define SYNC_ERROR_ID_PERM_NOT_SYNCABLE         8                   // SYNC_ERROR_LEVEL_FILE
#define SYNC_ERROR_ID_UPDATE_TO_READ_ONLY_REPO  9                   // SYNC_ERROR_LEVEL_FILE
#define SYNC_ERROR_ID_ACCESS_DENIED             10                  // SYNC_ERROR_LEVEL_REPO
#define SYNC_ERROR_ID_NO_WRITE_PERMISSION       11                  // SYNC_ERROR_LEVEL_REPO
#define SYNC_ERROR_ID_QUOTA_FULL                12                  // SYNC_ERROR_LEVEL_REPO
#define SYNC_ERROR_ID_NETWORK                   13                  // SYNC_ERROR_LEVEL_NETWORK
#define SYNC_ERROR_ID_RESOLVE_PROXY             14                  // SYNC_ERROR_LEVEL_NETWORK
#define SYNC_ERROR_ID_RESOLVE_HOST              15                  // SYNC_ERROR_LEVEL_NETWORK
#define SYNC_ERROR_ID_CONNECT                   16                  // SYNC_ERROR_LEVEL_NETWORK
#define SYNC_ERROR_ID_SSL                       17                  // SYNC_ERROR_LEVEL_NETWORK
#define SYNC_ERROR_ID_TX                        18                  // SYNC_ERROR_LEVEL_NETWORK
#define SYNC_ERROR_ID_TX_TIMEOUT                19                  // SYNC_ERROR_LEVEL_NETWORK
#define SYNC_ERROR_ID_UNHANDLED_REDIRECT        20                  // SYNC_ERROR_LEVEL_NETWORK
#define SYNC_ERROR_ID_SERVER                    21                  // SYNC_ERROR_LEVEL_REPO
#define SYNC_ERROR_ID_LOCAL_DATA_CORRUPT        22                  // SYNC_ERROR_LEVEL_REPO
#define SYNC_ERROR_ID_WRITE_LOCAL_DATA          23                  // SYNC_ERROR_LEVEL_REPO
#define SYNC_ERROR_ID_SERVER_REPO_DELETED       24                  // SYNC_ERROR_LEVEL_REPO
#define SYNC_ERROR_ID_SERVER_REPO_CORRUPT       25                  // SYNC_ERROR_LEVEL_REPO
#define SYNC_ERROR_ID_NOT_ENOUGH_MEMORY         26                  // SYNC_ERROR_LEVEL_REPO
#define SYNC_ERROR_ID_CONFLICT                  27                  // SYNC_ERROR_LEVEL_FILE
#define SYNC_ERROR_ID_GENERAL_ERROR             28                  // SYNC_ERROR_LEVEL_REPO
#define SYNC_ERROR_ID_NO_ERROR                  29                  // SYNC_ERROR_LEVEL_REPO

#endif //SEAFILE_CLIENT_SEAFILE_ERROR_H
