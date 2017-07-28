#include "finder-sync/finder-sync-host.h"
#include "finder-sync/finder-sync-listener.h"

#include <libkern/OSAtomic.h>
#include <mach/mach.h>
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
// listener related
static NSThread *finder_sync_listener_thread_ = nil;
// atomic value
static volatile int32_t finder_sync_started_ = 0;
static FinderSyncListener *finder_sync_listener_ = nil;
static std::unique_ptr<FinderSyncHost, QtLaterDeleter> finder_sync_host_;
static constexpr uint32_t kFinderSyncProtocolVersion = 0x00000004;

static void handleGetFileStatus(mach_msg_command_rcv_t* msg) {
    // generate reply
    mach_port_t port = msg->header.msgh_remote_port;
    if (!port)
        return;
    mach_msg_file_status_send_t reply_msg;
    bzero(&reply_msg, sizeof(mach_msg_header_t));
    mach_msg_header_t *reply_msg_header =
      reinterpret_cast<mach_msg_header_t*>(&reply_msg);
    reply_msg_header->msgh_id = msg->header.msgh_id + 1;
    reply_msg_header->msgh_size = sizeof(mach_msg_file_status_send_t);
    reply_msg_header->msgh_local_port = MACH_PORT_NULL;
    reply_msg_header->msgh_remote_port = port;
    reply_msg_header->msgh_bits = MACH_MSGH_BITS_REMOTE(msg->header.msgh_bits);
    reply_msg.status = finder_sync_host_->getFileStatus(msg->repo, msg->body);

    // send the reply
    kern_return_t kr = mach_msg_send(reply_msg_header);
    if (kr != MACH_MSG_SUCCESS) {
        qDebug("[FinderSync] mach error %s", mach_error_string(kr));
        // qWarning("[FinderSync] failed to send reply");
    }

    // destroy
    mach_msg_destroy(reply_msg_header);
}

static void handleGetWatchSet(mach_msg_command_rcv_t* msg) {
    // generate reply
    mach_port_t port = msg->header.msgh_remote_port;
    if (!port)
        return;
    utils::BufferArray reply_msg =
      finder_sync_host_->getWatchSet(sizeof(mach_msg_header_t));
    bzero(reply_msg.data(), sizeof(mach_msg_header_t));
    mach_msg_header_t *reply_msg_header =
      reinterpret_cast<mach_msg_header_t*>(reply_msg.data());
    reply_msg_header->msgh_id = msg->header.msgh_id + 1;
    reply_msg_header->msgh_size = reply_msg.size();
    reply_msg_header->msgh_local_port = MACH_PORT_NULL;
    reply_msg_header->msgh_remote_port = port;
    reply_msg_header->msgh_bits = MACH_MSGH_BITS_REMOTE(msg->header.msgh_bits);

    // send the reply
    kern_return_t kr = mach_msg_send(reply_msg_header);
    if (kr != MACH_MSG_SUCCESS) {
        qDebug("[FinderSync] mach error %s", mach_error_string(kr));
        // qWarning("[FinderSync] failed to send reply");
    }

    // destroy
    mach_msg_destroy(reply_msg_header);
}

@interface FinderSyncListener ()
@property(readwrite, nonatomic, strong) NSPort *listenerPort;
@end

@implementation FinderSyncListener
- (instancetype)init {
    self = [super init];
    self.listenerPort = nil;
    return self;
}
- (void)start {
    NSRunLoop *runLoop = [NSRunLoop currentRunLoop];
    mach_port_t port = MACH_PORT_NULL;

    kern_return_t kr =
        mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &port);
    if (kr != KERN_SUCCESS) {
        qWarning("[FinderSync] failed to allocate mach port");
        qDebug("[FinderSync] mach error %s", mach_error_string(kr));
        kr = mach_port_mod_refs(mach_task_self(), port, MACH_PORT_RIGHT_RECEIVE,
                                -1);
        if (kr != KERN_SUCCESS) {
            qDebug("[FinderSync] failed to deallocate mach port");
            qDebug("[FinderSync] mach error %s", mach_error_string(kr));
        }
        return;
    }

    kr = mach_port_insert_right(mach_task_self(), port, port,
                                MACH_MSG_TYPE_MAKE_SEND);
    if (kr != KERN_SUCCESS) {
        qWarning("[FinderSync] failed to insert send right to mach port");
        qDebug("[FinderSync] mach error %s", mach_error_string(kr));
        kr = mach_port_mod_refs(mach_task_self(), port, MACH_PORT_RIGHT_RECEIVE,
                                -1);
        if (kr != KERN_SUCCESS) {
            qDebug("[FinderSync] failed to deallocate mach port");
            qDebug("[FinderSync] mach error %s", mach_error_string(kr));
        }
        qDebug("[FinderSync] failed to allocate send right tp local mach port");
        return;
    }
    self.listenerPort =
        [NSMachPort portWithMachPort:port
                             options:NSMachPortDeallocateReceiveRight];
    if (![[NSMachBootstrapServer sharedInstance]
            registerPort:self.listenerPort
                    name:kFinderSyncMachPort]) {
        [self.listenerPort invalidate];
        qWarning("[FinderSync] failed to register mach port");
        qDebug("[FinderSync] mach error %s", mach_error_string(kr));
        return;
    }
    qDebug("[FinderSync] mach port registered");
    [self.listenerPort setDelegate:self];
    [runLoop addPort:self.listenerPort forMode:NSDefaultRunLoopMode];
    while (finder_sync_started_)
        [runLoop runMode:NSDefaultRunLoopMode
              beforeDate:[NSDate distantFuture]];
    [self.listenerPort invalidate];
    qDebug("[FinderSync] mach port unregistered");
    kr = mach_port_deallocate(mach_task_self(), port);
    if (kr != KERN_SUCCESS) {
        qDebug("[FinderSync] failed to deallocate mach port %u", port);
        return;
    }
}
- (void)stop {
    CFRunLoopStop(CFRunLoopGetCurrent());
}

- (void)handleMachMessage:(void *)machMessage {
    mach_msg_command_rcv_t *msg =
        static_cast<mach_msg_command_rcv_t *>(machMessage);
    if (msg->header.msgh_size != sizeof(mach_msg_command_send_t)) {
        qWarning("[FinderSync] received msg with bad size %u", msg->header.msgh_size);
        mach_msg_destroy(&msg->header);
        return;
    }

    if (msg->version != kFinderSyncProtocolVersion) {
        NSLog(@"FinderSync incompatible message protocol version %u", msg->version);
        qWarning("[FinderSync] incompatible message protocol version %u", msg->version);
        mach_msg_destroy(&msg->header);
        // we need to quit before it cause more serious issue!
        finderSyncListenerStop();
        return;
    }

    switch (msg->command) {
    case GetWatchSet:
        handleGetWatchSet(msg);
        break;
    case DoShareLink:
        // handle DoShareLink
        QMetaObject::invokeMethod(finder_sync_host_.get(), "doShareLink",
                                  Qt::QueuedConnection,
                                  Q_ARG(QString, msg->body));
        break;
    case DoInternalLink:
        // handle DoShareLink
        QMetaObject::invokeMethod(finder_sync_host_.get(), "doInternalLink",
                                  Qt::QueuedConnection,
                                  Q_ARG(QString, msg->body));
        break;
    case DoGetFileStatus:
        handleGetFileStatus(msg);
        break;
    case DoLockFile:
        QMetaObject::invokeMethod(finder_sync_host_.get(), "doLockFile",
                                  Qt::QueuedConnection,
                                  Q_ARG(QString, msg->body),
                                  Q_ARG(bool, true));
        break;
    case DoUnlockFile:
        QMetaObject::invokeMethod(finder_sync_host_.get(), "doLockFile",
                                  Qt::QueuedConnection,
                                  Q_ARG(QString, msg->body),
                                  Q_ARG(bool, false));
        break;
    case DoShowFileHistory:
        QMetaObject::invokeMethod(finder_sync_host_.get(), "doShowFileHistory",
                                  Qt::QueuedConnection,
                                  Q_ARG(QString, msg->body));
        break;
    default:
        qWarning("[FinderSync] received unknown command %u", msg->command);
        break;
    }
    mach_msg_destroy(&msg->header);
}

@end

void finderSyncListenerStart() {
    if (!OSAtomicAdd32Barrier(0, &finder_sync_started_)) {
        // this value is used in different threads
        // keep it in atomic and guarenteed by barrier for safety
        OSAtomicIncrement32Barrier(&finder_sync_started_);

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
    if (OSAtomicAdd32Barrier(0, &finder_sync_started_)) {
        // this value is used in different threads
        // keep it in atomic and guarenteed by barrier for safety
        OSAtomicDecrement32Barrier(&finder_sync_started_);

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
