//
//  FinderSyncClient.h
//  seafile-client-fsplugin
//
//  Created by Chilledheart on 1/10/15.
//  Copyright (c) 2015 Haiwen. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#include <thread>
#include <mutex>

#include <vector>
#include <string>
#include "FinderSync.h"

enum PathStatus : uint32_t {
    SYNC_STATUS_NONE = 0,
    SYNC_STATUS_SYNCING,
    SYNC_STATUS_ERROR,
    SYNC_STATUS_IGNORED,
    SYNC_STATUS_SYNCED,
    SYNC_STATUS_READONLY,
    SYNC_STATUS_PAUSED,
    SYNC_STATUS_LOCKED,
    SYNC_STATUS_LOCKED_BY_ME,
    MAX_SYNC_STATUS,
};

struct LocalRepo {
    LocalRepo() = default;
    LocalRepo(const LocalRepo &) = delete;
    LocalRepo &operator=(const LocalRepo &) = delete;

    LocalRepo(LocalRepo &&) = default;
    LocalRepo &operator=(LocalRepo &&) = default;
    enum SyncState : uint8_t {
        SYNC_STATE_DISABLED = 0,
        SYNC_STATE_WAITING = 1,
        SYNC_STATE_INIT = 2,
        SYNC_STATE_ING = 3,
        SYNC_STATE_DONE = 4,
        SYNC_STATE_ERROR = 5,
        SYNC_STATE_UNKNOWN = 6,
        SYNC_STATE_UNSET = 7,
        MAX_SYNC_STATE,
    };
    LocalRepo(std::string &&r, std::string &&w, SyncState s, bool _internal_link_supported)
        : repo_id(std::move(r)), worktree(std::move(w)), status(s), internal_link_supported(_internal_link_supported) {}
    LocalRepo(const std::string &r, const std::string &w, SyncState s, bool _internal_link_supported)
        : repo_id(r), worktree(w), status(s), internal_link_supported(_internal_link_supported) {}
    std::string repo_id;
    std::string worktree;
    SyncState status;
    bool internal_link_supported;
    friend bool operator==(const LocalRepo &lhs, const LocalRepo &rhs) {
        return lhs.repo_id == rhs.repo_id;
    }
    friend bool operator!=(const LocalRepo &lhs, const LocalRepo &rhs) {
        return !(lhs == rhs);
    }
};

class FinderSyncClient {
  public:
    enum CommandType : uint32_t {
        GetWatchSet = 0,
        DoShareLink = 1,
        DoGetFileStatus = 2,
        DoInternalLink = 3,
        DoLockFile = 4,
        DoUnlockFile = 5,
        DoShowFileHistory = 6,
        DoShowFileLockedBy = 7,
        DoGetUploadLink = 8,
        DoShareToUser = 9,
        DoShareToGroup = 10,
    };

    FinderSyncClient(FinderSync *parent);
    ~FinderSyncClient();
    void getWatchSet();
    void doSendCommandWithPath(CommandType command, const char *fileName);
    void doGetFileStatus(const char *repo, const char *fileName);

  private:
    bool connect();
    void connectionBecomeInvalid();
    FinderSync __weak *parent_;
    mach_port_t local_port_;
    mach_port_t remote_port_;
};
