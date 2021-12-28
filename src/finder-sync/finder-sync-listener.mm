#include "finder-sync/finder-sync-host.h"
#include "finder-sync/finder-sync-listener.h"

/* Mentioned in OSAtomic.h https://opensource.apple.com/source/libplatform/libplatform-125/include/libkern/OSAtomicDeprecated.h.auto.html
 * These are deprecated legacy interfaces for atomic operations.
 * The C11 interfaces in <stdatomic.h> resp. C++11 interfaces in <atomic>
 * should be used instead. */
#include <atomic>
#include <mach/mach.h>
#include <servers/bootstrap.h>
#import <Cocoa/Cocoa.h>

#if !__has_feature(objc_arc)
#error this file must be built with ARC support
#endif

@interface FinderSyncListener : NSObject <NSMachPortDelegate>
@end
//
// MachPort Message
// - mach_msg_header_t command
// - uint32_t version
// - uint32_t command
// - body
// - mach_msg_trailer_t trailer (for rcv only)
//
//

namespace {
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

struct mach_msg_command_send_t {
    mach_msg_header_t header;
    uint32_t version;
    uint32_t command;
    char repo[36];
    char body[kPathMaxSize];
};

struct mach_msg_command_rcv_t {
    mach_msg_header_t header;
    uint32_t version;
    uint32_t command;
    char repo[36];
    char body[kPathMaxSize];
    mach_msg_trailer_t trailer;
};

struct mach_msg_file_status_send_t {
    mach_msg_header_t header;
    uint32_t version;
    uint32_t command;
    uint32_t status;
};

struct QtLaterDeleter {
public:
  void operator()(QObject *ptr) {
    ptr->deleteLater();
  }
};
} // anonymous namespace

static NSString *const kFinderSyncMachPort = @"com.seafile.seafile-client.findersync.machport";
static constexpr uint32_t kFinderSyncProtocolVersion = 0x00000004;

// listener related
static NSThread *finder_sync_listener_thread_ = nil;
static std::atomic_int32_t finder_sync_started_;
static FinderSyncListener *finder_sync_listener_ = nil;

static std::unique_ptr<FinderSyncHost, QtLaterDeleter> finder_sync_host_;

static void handleGetFileStatus(mach_msg_command_rcv_t* msg) {
    // generate reply
    kern_return_t kr;
    mach_port_t remote_port = msg->header.msgh_remote_port;
    mach_msg_file_status_send_t reply_msg;
    mach_msg_header_t *reply_msg_header;

    bzero(&reply_msg, sizeof(mach_msg_header_t));

    if (!remote_port) {
        qWarning("[FinderSync] failed to send get file status reply due to invalid remote port");
        return;
    }

    // init the reply
    reply_msg_header = reinterpret_cast<mach_msg_header_t*>(&reply_msg);
    reply_msg_header->msgh_id = msg->header.msgh_id + 1;
    reply_msg_header->msgh_size = sizeof(mach_msg_file_status_send_t);
    reply_msg_header->msgh_local_port = MACH_PORT_NULL;
    reply_msg_header->msgh_remote_port = remote_port;
    reply_msg_header->msgh_bits = MACH_MSGH_BITS_REMOTE(msg->header.msgh_bits);

    // init the reply body
    reply_msg.status = finder_sync_host_->getFileStatus(msg->repo, msg->body);

    // send the reply
    kr = mach_msg_send(reply_msg_header);
    if (kr != MACH_MSG_SUCCESS) {
        qWarning("[FinderSync] failed to send get file status reply to remote port %d due to %s",
                 remote_port, mach_error_string(kr));
    }

    // destroy the reply
    mach_msg_destroy(reply_msg_header);
}

static void handleGetWatchSet(mach_msg_command_rcv_t* msg) {
    // generate reply
    kern_return_t kr;
    mach_port_t remote_port = msg->header.msgh_remote_port;
    mach_msg_header_t *reply_msg_header;
    utils::BufferArray reply_msg;

    reply_msg = finder_sync_host_->getWatchSet(sizeof(mach_msg_header_t));
    bzero(reply_msg.data(), sizeof(mach_msg_header_t));

    if (!remote_port) {
        qWarning("[FinderSync] failed to send get watch set reply due to invalid remote port");
        return;
    }

    // init the reply
    reply_msg_header = reinterpret_cast<mach_msg_header_t*>(reply_msg.data());
    reply_msg_header->msgh_id = msg->header.msgh_id + 1;
    reply_msg_header->msgh_size = reply_msg.size();
    reply_msg_header->msgh_local_port = MACH_PORT_NULL;
    reply_msg_header->msgh_remote_port = remote_port;
    reply_msg_header->msgh_bits = MACH_MSGH_BITS_REMOTE(msg->header.msgh_bits);

    // send the reply
    kr = mach_msg_send(reply_msg_header);
    if (kr != MACH_MSG_SUCCESS) {
        qWarning("[FinderSync] failed to send get watch set reply to remote port %d due to %s",
                 remote_port, mach_error_string(kr));
    }

    // destroy the reply
    mach_msg_destroy(reply_msg_header);
}

@interface FinderSyncListener ()
@property(readwrite, nonatomic, strong) NSPort *listenerPort;
@end

@implementation FinderSyncListener

- (instancetype)init {
    self = [super init];
    self.listenerPort = nil;

    Q_ASSERT(kFinderSyncMachPort.length < BOOTSTRAP_MAX_NAME_LEN && \
        "mach port name too long");

    return self;
}

- (void)start {
    kern_return_t kr;
    mach_port_t port = MACH_PORT_NULL;
    NSRunLoop *runLoop = [NSRunLoop currentRunLoop];

    // Allocate mach port resource
    kr = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &port);

    if (kr != KERN_SUCCESS) {
        qWarning("[FinderSync] failed to allocate mach port due to %s",
                 mach_error_string(kr));

        kr = mach_port_mod_refs(mach_task_self(), port, MACH_PORT_RIGHT_RECEIVE,
                                -1);
        if (kr != KERN_SUCCESS) {
            qWarning("[FinderSync] failed to remove receive right to due to error %s",
                     mach_error_string(kr));
        }
        goto done;
    }

    // Insert send right to mach port
    kr = mach_port_insert_right(mach_task_self(), port, port,
                                MACH_MSG_TYPE_MAKE_SEND);

    if (kr != KERN_SUCCESS) {
        qWarning("[FinderSync] failed to insert send right to local mach port due to error %s",
                 mach_error_string(kr));

        kr = mach_port_mod_refs(mach_task_self(), port, MACH_PORT_RIGHT_RECEIVE,
                                -1);

        if (kr != KERN_SUCCESS) {
            qWarning("[FinderSync] failed to remove receive right to due to error %s",
                     mach_error_string(kr));
        }

        goto done;
    }

    // Register mach port
    self.listenerPort =
        [NSMachPort portWithMachPort:port
                             options:NSMachPortDeallocateReceiveRight];
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    if (![[NSMachBootstrapServer sharedInstance]
            registerPort:self.listenerPort
                    name:kFinderSyncMachPort]) {
        qWarning("[FinderSync] failed to register mach port due to error %s",
                 mach_error_string(kr));
        goto done;
    }
#pragma clang diagnostic pop

    qDebug("[FinderSync] listen port registered");

    // Set up mach port delegate and run loop source
    [self.listenerPort setDelegate:self];
    [runLoop addPort:self.listenerPort forMode:NSDefaultRunLoopMode];

    qDebug("[FinderSync] listen port source added");

    // Run Loop
    while (finder_sync_started_.load())
        [runLoop runMode:NSDefaultRunLoopMode
              beforeDate:[NSDate distantFuture]];

done:
    if (self.listenerPort) {
        [self.listenerPort invalidate];
        self.listenerPort = nil;
    }

    kr = mach_port_deallocate(mach_task_self(), port);
    if (kr != KERN_SUCCESS) {
        qWarning("[FinderSync] failed to deallocate mach port due to error %s",
                 mach_error_string(kr));
    }

    qDebug("[FinderSync] listen port unregistered");
}

- (void)stop {
    CFRunLoopStop(CFRunLoopGetCurrent());
}

- (void)handleMachMessage:(void *)machMessage {
    mach_msg_command_rcv_t *recv_msg =
        static_cast<mach_msg_command_rcv_t *>(machMessage);

    // check mach msg size
    if (recv_msg->header.msgh_size != sizeof(mach_msg_command_send_t)) {
        qWarning("[FinderSync] received msg with bad size %u",
                 recv_msg->header.msgh_size);
        goto done;
    }

    // check mach msg ipc protocol content
    if (recv_msg->version != kFinderSyncProtocolVersion) {
        qWarning("[FinderSync] incompatible message protocol version %u",
                 recv_msg->version);

        // we need to quit before it causes more serious issue!
        finderSyncListenerStop();

        goto done;
    }

    // process mach msg
    switch (recv_msg->command) {
    case GetWatchSet:
        // handle GetWatchSet
        handleGetWatchSet(recv_msg);
        break;

    case DoShareLink:
        // handle DoShareLink
        QMetaObject::invokeMethod(finder_sync_host_.get(), "doShareLink",
                                  Qt::QueuedConnection,
                                  Q_ARG(QString, recv_msg->body));
        break;

    case DoInternalLink:
        // handle DoShareLink
        QMetaObject::invokeMethod(finder_sync_host_.get(), "doInternalLink",
                                  Qt::QueuedConnection,
                                  Q_ARG(QString, recv_msg->body));
        break;

    case DoGetFileStatus:
        // handle GetFileStatus
        handleGetFileStatus(recv_msg);
        break;

    case DoLockFile:
        // handle DoLockFile
        QMetaObject::invokeMethod(finder_sync_host_.get(), "doLockFile",
                                  Qt::QueuedConnection,
                                  Q_ARG(QString, recv_msg->body),
                                  Q_ARG(bool, true));
        break;

    case DoUnlockFile:
        // handle DoUnlockFile
        QMetaObject::invokeMethod(finder_sync_host_.get(), "doLockFile",
                                  Qt::QueuedConnection,
                                  Q_ARG(QString, recv_msg->body),
                                  Q_ARG(bool, false));
        break;

    case DoShowFileHistory:
        // handle DoShowFileHistory
        QMetaObject::invokeMethod(finder_sync_host_.get(), "doShowFileHistory",
                                  Qt::QueuedConnection,
                                  Q_ARG(QString, recv_msg->body));
        break;

    case DoShowFileLockedBy:
        // handle DoShowFileLockedBy
        QMetaObject::invokeMethod(finder_sync_host_.get(), "doShowFileLockedBy",
                                  Qt::QueuedConnection,
                                  Q_ARG(QString, recv_msg->body));
        break;

    case DoShareToUser:
        // handle DoShareToUser
        QMetaObject::invokeMethod(finder_sync_host_.get(), "doShareToUser",
                                  Qt::QueuedConnection,
                                  Q_ARG(QString, recv_msg->body));
        break;

    case DoShareToGroup:
        // handle DoShareToGroup
        QMetaObject::invokeMethod(finder_sync_host_.get(), "doShareToGroup",
                                  Qt::QueuedConnection,
                                  Q_ARG(QString, recv_msg->body));
        break;

    case DoGetUploadLink:
        // handle DoGetUploadLink
        QMetaObject::invokeMethod(finder_sync_host_.get(), "doGetUploadLink",
                                  Qt::QueuedConnection,
                                  Q_ARG(QString, recv_msg->body));
        break;

    default:
        qWarning("[FinderSync] received unknown command %u", recv_msg->command);
        break;
    }

done:
    mach_msg_destroy(&recv_msg->header);
}

@end

void finderSyncListenerStart() {
    if (!finder_sync_started_.load()) {
        // this value is used in different threads
        // keep it in atomic and guarenteed by barrier for safety
        finder_sync_started_.fetch_add(1);

        dispatch_async(dispatch_get_main_queue(), ^{
            finder_sync_host_.reset(new FinderSyncHost);
            finder_sync_listener_ = [[FinderSyncListener alloc] init];
            finder_sync_listener_thread_ =
                [[NSThread alloc] initWithTarget:finder_sync_listener_
                                        selector:@selector(start)
                                          object:nil];
            [finder_sync_listener_thread_ start];
        });
    }
}

void finderSyncListenerStop() {
    if (finder_sync_started_.load()) {
        // this value is used in different threads
        // keep it in atomic and guarenteed by barrier for safety
        finder_sync_started_.fetch_sub(1);

        // tell finder_sync_listener_ to exit
        [finder_sync_listener_ performSelector:@selector(stop)
                                      onThread:finder_sync_listener_thread_
                                    withObject:nil
                                 waitUntilDone:NO];
        dispatch_async(dispatch_get_main_queue(), ^{
             finder_sync_host_.reset();
        });
    }
}
