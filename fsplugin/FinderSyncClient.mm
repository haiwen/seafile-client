//
//  FinderSyncClient.m
//  seafile-client-fsplugin
//
//  Created by Chilledheart on 1/10/15.
//  Copyright (c) 2015 Haiwen. All rights reserved.
//

#import "FinderSyncClient.h"
#include <cstdint>
#include <cassert>
#include <sstream>
#include <iostream>
#include <string>
#include <servers/bootstrap.h>
#include "../src/utils/stl.h"

#if !__has_feature(objc_arc)
#error this file must be built with ARC support
#endif

static NSString *const kFinderSyncMachPort =
    @"com.seafile.seafile-client.findersync.machport";

static constexpr int kPathMaxSize = 1024;
static constexpr uint32_t kFinderSyncProtocolVersion = 0x00000004;
static constexpr int kMachMsgTimeoutMS = 100;
static volatile int32_t message_id_ =
    100; // we start from 100, the number below than 100 is reserved

typedef std::vector<std::string> stringlist;

stringlist split_string(const std::string& v, char delim) {
    using namespace std;
    stringlist strings;
    istringstream ss(v);
    string s;
    while (getline(ss, s, delim)) {
        strings.push_back(s);
    }
    return strings;
}

//
// MachPort Message
// - mach_msg_header_t command
// - uint32_t version
// - uint32_t command
// - body
// - mach_msg_trailer_t trailer (for rcv only)
//
//

// buffer
// <-char[36]-><----- char? ------------>1   1      1
// <-repo_id--><---- [worktree name] --->0<[status]>0
static std::vector<LocalRepo> *deserializeWatchSet(const char *buffer,
                                                   uint32_t size) {
    std::vector<LocalRepo> *repos = new std::vector<LocalRepo>();
    const char *const end = buffer + size - 1;
    const char *pos;
    while (buffer != end) {
        unsigned worktree_size;
        uint8_t status;
        const char *repo_id = buffer;
        buffer += 36;
        pos = buffer;

        while (*pos != '\0' && pos != end)
            ++pos;
        worktree_size = pos - buffer;
        pos += 2;
        if (pos > end || *pos != '\0')
            break;

        status = *(pos - 1);
        if (status >= LocalRepo::MAX_SYNC_STATE) {
            status = LocalRepo::SYNC_STATE_UNSET;
        }
        bool internal_link_supported = false;
        std::string worktree = std::string(buffer, worktree_size);
        // NSLog(@"full line = %s", worktree.c_str());
        stringlist parts = split_string(worktree, '\t');
        if (parts.size() > 1) {
            worktree = parts[0];
            // NSLog(@"worktree = %s", worktree.c_str());
            internal_link_supported = parts[1] == "internal-link-supported";
        }

        repos->emplace_back(std::string(repo_id, 36),
                            worktree,
                            static_cast<LocalRepo::SyncState>(status),
                            internal_link_supported);
        buffer = ++pos;
    }
    return repos;
}

struct mach_msg_command_send_t {
    mach_msg_header_t header;
    uint32_t version;
    uint32_t command;
    // used only in DoShareLink
    char repo[36];
    char body[kPathMaxSize];
};

struct mach_msg_file_status_rcv_t {
    mach_msg_header_t header;
    uint32_t version;
    uint32_t command;
    uint32_t status;
    mach_msg_trailer_t trailer;
};

FinderSyncClient::FinderSyncClient(FinderSync *parent)
    : parent_(parent), local_port_(MACH_PORT_NULL),
      remote_port_(MACH_PORT_NULL) {
    assert(kFinderSyncMachPort.length < BOOTSTRAP_MAX_NAME_LEN && \
           @"mach port name too long");
}

FinderSyncClient::~FinderSyncClient() {
    kern_return_t kr;

    if (local_port_ != MACH_PORT_NULL) {
        kr = mach_port_mod_refs(mach_task_self(), local_port_,
                                MACH_PORT_RIGHT_RECEIVE, -1);

        if (kr != KERN_SUCCESS) {
            NSLog(@"failed to remove receive right to due to error %s",
                  mach_error_string(kr));
        }
    }

    if (remote_port_ != MACH_PORT_NULL) {
        kr = mach_port_deallocate(mach_task_self(), remote_port_);

        if (kr != KERN_SUCCESS) {
            NSLog(@"failed to deallocate mach port due to error %s",
                  mach_error_string(kr));
        }
    }

    NSLog(@"disconnecting from Seafile Client");
}

void FinderSyncClient::connectionBecomeInvalid() {
    kern_return_t kr;

    /* clean up old connection stage! */
    if (local_port_ != MACH_PORT_NULL) {
        kr = mach_port_mod_refs(mach_task_self(), local_port_,
                                MACH_PORT_RIGHT_RECEIVE, -1);

        if (kr != KERN_SUCCESS) {
            NSLog(@"failed to remove receive right to due to error %s",
                  mach_error_string(kr));
        }

        local_port_ = MACH_PORT_NULL;
    }

    if (remote_port_ != MACH_PORT_NULL) {
        kr = mach_port_deallocate(mach_task_self(), remote_port_);

        if (kr != KERN_SUCCESS) {
            NSLog(@"failed to deallocate mach port due to error %s",
                  mach_error_string(kr));
        }

        dispatch_async(dispatch_get_main_queue(), ^{
            [parent_ updateWatchSet:nil];
        });

        remote_port_ = MACH_PORT_NULL;
    }

    NSLog(@"lost connection with Seafile Client");
}

bool FinderSyncClient::connect() {
    kern_return_t kr;

    if (local_port_ == MACH_PORT_NULL) {
        // Create a local port.

        kr = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE,
                                &local_port_);

        if (kr != KERN_SUCCESS) {
            NSLog(@"failed to create connection due to %s",
                  mach_error_string(kr));
            return false;
        }
    }

    if (remote_port_ == MACH_PORT_NULL) {
        // Connect to the mach_port

        kr = bootstrap_look_up(bootstrap_port,
            [kFinderSyncMachPort cStringUsingEncoding:NSASCIIStringEncoding],
            &remote_port_);

        if (kr != BOOTSTRAP_SUCCESS) {
            NSLog(@"failed to lookup remote port port due to %s",
                  mach_error_string(kr));
            return false;
        }
    }

    NSLog(@"connected to Seafile Client");

    return true;
}

void FinderSyncClient::getWatchSet() {
    kern_return_t kr;
    mach_msg_command_send_t send_msg;
    int32_t recv_msgh_id;
    utils::BufferArray recv_msg;
    mach_msg_header_t *recv_msg_header;

    const char *body;
    uint32_t body_size;
    std::vector<LocalRepo> *repos;

    // Zero send and receive buffer
    recv_msg.resize(4096);
    bzero(&send_msg, sizeof(mach_msg_header_t));
    recv_msg_header = reinterpret_cast<mach_msg_header_t *>(recv_msg.data());
    bzero(recv_msg_header, sizeof(*recv_msg_header));

    if ([NSThread isMainThread]) {
        NSLog(@"%s isn't supported to be called from main thread",
              __PRETTY_FUNCTION__);
        return;
    }

    if (!connect()) {
        NSLog(@"failed to establish connection to Seafile Client");
        goto failed;
    }

    recv_msgh_id = OSAtomicAdd32(2, &message_id_) - 1;

    // Initial send message
    send_msg.header.msgh_id = recv_msgh_id - 1;
    send_msg.header.msgh_local_port = local_port_;
    send_msg.header.msgh_remote_port = remote_port_;
    send_msg.header.msgh_size = sizeof(send_msg);
    send_msg.header.msgh_bits =
        MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND, MACH_MSG_TYPE_MAKE_SEND_ONCE);

    // Initial send message body
    send_msg.version = kFinderSyncProtocolVersion;
    send_msg.command = GetWatchSet;

    // Send a message and wait for the reply
    kr = mach_msg(&send_msg.header,                  /* header*/
                  MACH_SEND_MSG | MACH_SEND_TIMEOUT, /*option*/
                  sizeof(send_msg),                  /*send size*/
                  0,                                 /*receive size*/
                  local_port_,                       /*receive port*/
                  kMachMsgTimeoutMS,                 /*timeout, in milliseconds*/
                  MACH_PORT_NULL);                   /*no notification*/

    if (kr != MACH_MSG_SUCCESS) {
        if (kr == MACH_SEND_TIMED_OUT) {
            NSLog(@"timed out while trying to send get watch set request.");
            goto failed;
        }

        if (kr == MACH_SEND_INVALID_DEST) {
            NSLog(@"failed to send get watch set request to due to a port that we don't possess a send right to.");
            goto failed;
        }

        NSLog(@"failed to send get watch set request to Seafile Client due to %s",
              mach_error_string(kr));
        goto failed;
    }

    // Recover the send msg resource
    mach_msg_destroy(&send_msg.header);

    // Initial reply mach msg
    recv_msg_header->msgh_local_port = local_port_;
    recv_msg_header->msgh_remote_port = remote_port_;
    // recv_msg.header.msgh_size = sizeof(recv_msg);

    // Receive the reply
    kr = mach_msg(recv_msg_header,                                  /* header*/
                  MACH_RCV_MSG | MACH_RCV_TIMEOUT | MACH_RCV_LARGE, /*option*/
                  0,                                                /*send size*/
                  recv_msg.size(),                                  /*receive size*/
                  local_port_,                                      /*receive port*/
                  kMachMsgTimeoutMS,                                /*timeout, in milliseconds*/
                  MACH_PORT_NULL);                                  /*no notification*/

    // Retry if the receive buffer is too small
    if (kr == MACH_RCV_TOO_LARGE) {
        recv_msg.resize(recv_msg_header->msgh_size +
                        sizeof(mach_msg_trailer_t));
        recv_msg_header =
            reinterpret_cast<mach_msg_header_t *>(recv_msg.data());

        kr = mach_msg(recv_msg_header,                 /* header*/
                      MACH_RCV_MSG | MACH_RCV_TIMEOUT, /*option*/
                      0,                               /*send size*/
                      recv_msg.size(),                 /*receive size*/
                      local_port_,                     /*receive port*/
                      kMachMsgTimeoutMS,               /*timeout, in milliseconds*/
                      MACH_PORT_NULL);                 /*no notification*/
    }

    // Check mach recv status
    if (kr != MACH_MSG_SUCCESS) {
        NSLog(@"failed to receive get watch set request, due to %s",
              mach_error_string(kr));
        goto failed;
    }

    // Check mach msg recv id
    if (recv_msg_header->msgh_id != recv_msgh_id) {
        NSLog(@"received unmatched mach message id %d, expected %d",
              recv_msg_header->msgh_id, recv_msgh_id);
        goto failed;
    }

    // Copy to heap before starting block
    body = recv_msg.data() + sizeof(mach_msg_header_t);
    body_size = (recv_msg_header->msgh_size - sizeof(mach_msg_header_t));
    repos = deserializeWatchSet(body, body_size);

    // Dispatch to the main queue for UI change
    dispatch_async(dispatch_get_main_queue(), ^{
        [parent_ updateWatchSet:repos];
        delete repos;
    });

    // Recover the receive msg resource
    mach_msg_destroy(recv_msg_header);

    return;

failed:
    // Recover the mach msg resource
    mach_msg_destroy(&send_msg.header);
    mach_msg_destroy(recv_msg_header);

    connectionBecomeInvalid();
}

void FinderSyncClient::doSendCommandWithPath(CommandType command,
                                             const char *fileName) {
    kern_return_t kr;
    mach_msg_command_send_t send_msg;

    bzero(&send_msg, sizeof(mach_msg_header_t));

    if ([NSThread isMainThread]) {
        NSLog(@"%s isn't supported to be called from main thread",
              __PRETTY_FUNCTION__);
        return;
    }

    if (!connect()) {
        NSLog(@"failed to establish connection to Seafile Client");
        goto failed;
    }

    // Initial send mach message
    send_msg.header.msgh_id = OSAtomicIncrement32(&message_id_) - 1;
    send_msg.header.msgh_local_port = MACH_PORT_NULL;
    send_msg.header.msgh_remote_port = remote_port_;
    send_msg.header.msgh_size = sizeof(send_msg);
    send_msg.header.msgh_bits = MACH_MSGH_BITS_REMOTE(MACH_MSG_TYPE_COPY_SEND);

    // Initial send mach message body
    strncpy(send_msg.body, fileName, kPathMaxSize);
    send_msg.version = kFinderSyncProtocolVersion;
    send_msg.command = command;

    // Send a message only
    kr = mach_msg_send(&send_msg.header);

    if (kr != MACH_MSG_SUCCESS) {
        if (kr == MACH_SEND_TIMED_OUT) {
            NSLog(@"timed out while trying to send sharing link request for file %s.", fileName);
            goto failed;
        }

        if (kr == MACH_SEND_INVALID_DEST) {
            NSLog(@"failed to send sharing link request for file %s due to a port that we don't possess a send right to.", fileName);
            goto failed;
        }

        NSLog(@"failed to send sharing link request for file %s due to %s message id %d", fileName, mach_error_string(kr),
              send_msg.header.msgh_id);
        goto failed;
    }

    // Recover the send msg resource
    mach_msg_destroy(&send_msg.header);

    return;

failed:
    // Recover the mach msg resource
    mach_msg_destroy(&send_msg.header);

    connectionBecomeInvalid();
}

void FinderSyncClient::doGetFileStatus(const char *repo, const char *fileName) {
    kern_return_t kr;
    mach_msg_command_send_t send_msg;
    int32_t recv_msgh_id;
    mach_msg_file_status_rcv_t recv_msg;
    mach_msg_header_t *recv_msg_header;
    uint32_t status;
    std::string *repo_id = new std::string;
    std::string *path = new std::string;

    // Zero send and receive buffer
    bzero(&send_msg, sizeof(mach_msg_header_t));
    recv_msg_header = reinterpret_cast<mach_msg_header_t *>(&recv_msg);
    bzero(recv_msg_header, sizeof(*recv_msg_header));

    if ([NSThread isMainThread]) {
        NSLog(@"%s isn't supported to be called from main thread",
              __PRETTY_FUNCTION__);
        return;
    }

    if (!connect()) {
        NSLog(@"failed to establish connection to Seafile Client");
        goto failed;
    }

    recv_msgh_id = OSAtomicAdd32(2, &message_id_) - 1;

    // Initial send message
    send_msg.header.msgh_id = recv_msgh_id - 1;
    send_msg.header.msgh_local_port = local_port_;
    send_msg.header.msgh_remote_port = remote_port_;
    send_msg.header.msgh_size = sizeof(send_msg);
    send_msg.header.msgh_bits =
        MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND, MACH_MSG_TYPE_MAKE_SEND_ONCE);

    // Initial send message body
    strncpy(send_msg.repo, repo, 36);
    strncpy(send_msg.body, fileName, kPathMaxSize);
    send_msg.version = kFinderSyncProtocolVersion;
    send_msg.command = DoGetFileStatus;

    // Send a message and wait for the reply
    kr = mach_msg(&send_msg.header,                  /* header*/
                  MACH_SEND_MSG | MACH_SEND_TIMEOUT, /*option*/
                  sizeof(send_msg),                  /*send size*/
                  0,                                 /*receive size*/
                  local_port_,                       /*receive port*/
                  kMachMsgTimeoutMS,                 /*timeout, in milliseconds*/
                  MACH_PORT_NULL);                   /*no notification*/

    if (kr != MACH_MSG_SUCCESS) {
        if (kr == MACH_SEND_TIMED_OUT) {
            NSLog(@"timed out while trying to send file status request for file %s.",
                  fileName);
            goto failed;
        }

        if (kr == MACH_SEND_INVALID_DEST) {
            NSLog(@"failed to send file status request for file to %s due to a port that we don't possess a send right to.",
                  fileName);
            goto failed;
        }

        NSLog(@"failed to send file status request for file %s due to %s message id %d",
              fileName, mach_error_string(kr),
              send_msg.header.msgh_id);
        goto failed;
    }

    // Recover the send msg resource
    mach_msg_destroy(&send_msg.header);

    // Initial reply mach msg
    recv_msg_header->msgh_local_port = local_port_;
    recv_msg_header->msgh_remote_port = remote_port_;
    // recv_msg.header.msgh_size = sizeof(recv_msg);

    // Receive the reply
    kr = mach_msg(recv_msg_header,                    /* header*/
                  MACH_RCV_MSG | MACH_RCV_TIMEOUT,    /*option*/
                  0,                                  /*send size*/
                  sizeof(mach_msg_file_status_rcv_t), /*receive size*/
                  local_port_,                        /*receive port*/
                  kMachMsgTimeoutMS,                  /*timeout, in milliseconds*/
                  MACH_PORT_NULL);                    /*no notification*/

    // Check mach recv status
    if (kr != MACH_MSG_SUCCESS) {
        NSLog(@"failed to receive file status reply for file %s due to %s",
              fileName, mach_error_string(kr));
        goto failed;
    }

    // Check mach msg recv id
    if (recv_msg_header->msgh_id != recv_msgh_id) {
        NSLog(@"failed to receive file status reply for file %s due to unmatched message id %d, expected %d",
              fileName, recv_msg_header->msgh_id,
              recv_msgh_id);
        goto failed;
    }

    // Deal with recevied messsage
    status = recv_msg.status;
    if (status >= PathStatus::MAX_SYNC_STATUS)
        status = PathStatus::SYNC_STATUS_NONE;

    // Copy to heap before starting block
    *repo_id = repo;
    *path = fileName;

    // Dispatch to the main queue for UI change
    dispatch_async(dispatch_get_main_queue(), ^{
        [parent_ updateFileStatus:repo_id->c_str()
                             path:path->c_str()
                           status:status];
        delete repo_id;
        delete path;
    });

    // Recover the recv msg resource
    mach_msg_destroy(recv_msg_header);

    return;

failed:
    // Recover the mach msg resource
    mach_msg_destroy(&send_msg.header);
    mach_msg_destroy(recv_msg_header);

    connectionBecomeInvalid();
}
