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

struct LocalRepo {
    LocalRepo() = default;
    LocalRepo(const LocalRepo &) = delete;
    LocalRepo(LocalRepo &&) = default;
    enum SyncState : uint32_t {
        SYNC_STATE_DISABLED = 0,
        SYNC_STATE_WAITING = 1,
        SYNC_STATE_INIT = 2,
        SYNC_STATE_ING = 3,
        SYNC_STATE_DONE = 4,
        SYNC_STATE_ERROR = 5,
        SYNC_STATE_UNKNOWN = 6,
        SYNC_STATE_UNSET = 7,
    };
    std::string worktree;
    SyncState status;
    friend bool operator==(const LocalRepo &lhs, const LocalRepo &rhs) {
        return lhs.worktree == rhs.worktree;
    }
    friend bool operator!=(const LocalRepo &lhs, const LocalRepo &rhs) {
        return !(lhs == rhs);
    }
};

class FinderSyncClient {
  public:
    FinderSyncClient(FinderSync *parent);
    ~FinderSyncClient();
    void getWatchSet();
    void doSharedLink(const char *fileName);

  private:
    bool connect();
    void connectionBecomeInvalid();
    FinderSync __weak *parent_;
    mach_port_t local_port_;
    mach_port_t remote_port_;
};
