//
//  FinderSyncClient.m
//  seafile-client-fsplugin
//
//  Created by Chilledheart on 1/10/15.
//  Copyright (c) 2015 Haiwen. All rights reserved.
//

#import "FinderSyncClient.h"
#include <cstdint>
#include <servers/bootstrap.h>
#include <mutex>

#if !__has_feature(objc_arc)
#error this file must be built with ARC support
#endif

static NSString *const kFinderSyncMachPort =
    @"com.seafile.seafile-client.findersync.machport";

static constexpr int kWatchDirMax = 100;
static constexpr int kPathMaxSize = 1024;
static std::mutex mach_msg_mutex_;
static constexpr uint32_t kFinderSyncProtocolVersion = 1;

//
// MachPort Message
// - mach_msg_header_t command
// - uint32_t version
// - uint32_t command
// - body
// - mach_msg_trailer_t trailer (for rcv only)
//
//

enum CommandType : uint32_t {
    GetWatchSet = 0,
    DoShareLink = 1,
};

struct mach_msg_command_send_t {
    mach_msg_header_t header;
    uint32_t version;
    uint32_t command;
    // used only in DoShareLink
    char body[kPathMaxSize];
};

struct watch_dir_t {
    char body[kPathMaxSize];
    uint32_t status;
};

struct mach_msg_watchdir_rcv_t {
    mach_msg_header_t header;
    watch_dir_t dirs[kWatchDirMax];
    mach_msg_trailer_t trailer;
};

FinderSyncClient::FinderSyncClient(FinderSync *parent)
    : parent_(parent), local_port_(MACH_PORT_NULL),
      remote_port_(MACH_PORT_NULL) {}

FinderSyncClient::~FinderSyncClient() {
    if (local_port_) {
        mach_port_mod_refs(mach_task_self(), local_port_,
                           MACH_PORT_RIGHT_RECEIVE, -1);
    }
    if (remote_port_) {
        NSLog(@"disconnecting from Seafile Client");
        mach_port_deallocate(mach_task_self(), remote_port_);
    }
}

void FinderSyncClient::connectionBecomeInvalid() {
    if (remote_port_) {
        NSLog(@"lost connection with Seafile Client");
        mach_port_deallocate(mach_task_self(), remote_port_);
        dispatch_async(dispatch_get_main_queue(), ^{
          [parent_ updateWatchSet:nil];
        });
        remote_port_ = MACH_PORT_NULL;
    }
}

bool FinderSyncClient::connect() {
    if (!local_port_) {
        // Create a local port.
        kern_return_t kr = mach_port_allocate(
            mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &local_port_);
        if (kr != KERN_SUCCESS) {
            NSLog(@"unable to create connection");
            return false;
        }
    }

    if (!remote_port_) {
        // connect to the mach_port
        kern_return_t kr = bootstrap_look_up(
            bootstrap_port,
            [kFinderSyncMachPort cStringUsingEncoding:NSASCIIStringEncoding],
            &remote_port_);

        if (kr != KERN_SUCCESS) {
            return false;
        }
        NSLog(@"connected to Seafile Client");
    }

    return true;
}

void FinderSyncClient::getWatchSet() {
    if ([NSThread isMainThread]) {
        NSLog(@"%s isn't supported to be called from main thread",
              __PRETTY_FUNCTION__);
        return;
    }
    std::lock_guard<std::mutex> lock(mach_msg_mutex_);
    if (!connect())
        return;
    mach_msg_command_send_t msg;
    bzero(&msg, sizeof(mach_msg_header_t));
    msg.header.msgh_id = 0;
    msg.header.msgh_local_port = local_port_;
    msg.header.msgh_remote_port = remote_port_;
    msg.header.msgh_size = sizeof(msg);
    msg.header.msgh_bits =
        MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND, MACH_MSG_TYPE_MAKE_SEND_ONCE);
    msg.version = kFinderSyncProtocolVersion;
    msg.command = GetWatchSet;
    // send a message and wait for the reply
    kern_return_t kr = mach_msg(&msg.header,                       /* header*/
                                MACH_SEND_MSG | MACH_SEND_TIMEOUT, /*option*/
                                sizeof(msg),                       /*send size*/
                                0,               /*receive size*/
                                local_port_,     /*receive port*/
                                100,             /*timeout, in milliseconds*/
                                MACH_PORT_NULL); /*no notification*/
    if (kr != MACH_MSG_SUCCESS) {
        NSLog(@"failed to send request to Seafile Client");
        NSLog(@"mach error %s", mach_error_string(kr));
        if (kr == MACH_SEND_INVALID_DEST)
            connectionBecomeInvalid();
        return;
    }

    mach_msg_watchdir_rcv_t recv_msg;
    bzero(&recv_msg, sizeof(mach_msg_header_t));
    recv_msg.header.msgh_local_port = local_port_;
    recv_msg.header.msgh_remote_port = remote_port_;
    // recv_msg.header.msgh_size = sizeof(recv_msg);
    // receive the reply
    kr = mach_msg(&recv_msg.header,                /* header*/
                  MACH_RCV_MSG | MACH_RCV_TIMEOUT, /*option*/
                  0,                               /*send size*/
                  sizeof(recv_msg),                /*receive size*/
                  local_port_,                     /*receive port*/
                  100,                             /*timeout, in milliseconds*/
                  MACH_PORT_NULL);                 /*no notification*/
    if (kr != MACH_MSG_SUCCESS) {
        NSLog(@"failed to receive Seafile Client's reply");
        NSLog(@"mach error %s", mach_error_string(kr));
        return;
    }
    size_t count = (recv_msg.header.msgh_size - sizeof(mach_msg_header_t)) /
                   sizeof(watch_dir_t);
    dispatch_async(dispatch_get_main_queue(), ^{
      std::vector<LocalRepo> repos;
      for (size_t i = 0; i != count; i++) {
          LocalRepo repo;
          repo.worktree = recv_msg.dirs[i].body;
          repo.status =
              static_cast<LocalRepo::SyncState>(recv_msg.dirs[i].status);
          repos.emplace_back(std::move(repo));
      }
      [parent_ updateWatchSet:&repos];
    });
}

void FinderSyncClient::doSharedLink(const char *fileName) {
    if ([NSThread isMainThread]) {
        NSLog(@"%s isn't supported to be called from main thread",
              __PRETTY_FUNCTION__);
        return;
    }
    std::lock_guard<std::mutex> lock(mach_msg_mutex_);
    if (!connect())
        return;
    mach_msg_command_send_t msg;
    bzero(&msg, sizeof(msg));
    msg.header.msgh_id = 1;
    msg.header.msgh_local_port = MACH_PORT_NULL;
    msg.header.msgh_remote_port = remote_port_;
    msg.header.msgh_size = sizeof(msg);
    msg.header.msgh_bits = MACH_MSGH_BITS_REMOTE(MACH_MSG_TYPE_COPY_SEND);
    strncpy(msg.body, fileName, kPathMaxSize);
    msg.version = kFinderSyncProtocolVersion;
    msg.command = DoShareLink;
    // send a message only
    kern_return_t kr = mach_msg_send(&msg.header);
    if (kr != MACH_MSG_SUCCESS) {
        NSLog(@"failed to send sharing link request for %s", fileName);
        NSLog(@"mach error %s", mach_error_string(kr));
        if (kr == MACH_SEND_INVALID_DEST)
            connectionBecomeInvalid();
        return;
    }
}
