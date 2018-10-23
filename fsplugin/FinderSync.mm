//
//  FinderSync.m
//  seafile-client-fsplugin
//
//  Created by Chilledheart on 1/10/15.
//  Copyright (c) 2015 Haiwen. All rights reserved.
//

#import "FinderSync.h"
#import "FinderSyncClient.h"
#include <utility>
#include <map>
#include <unordered_map>
#include <algorithm>

#if !__has_feature(objc_arc)
#error this file must be built with ARC support
#endif

@interface FinderSync ()

@property(readwrite, nonatomic, strong) NSTimer *update_watch_set_timer_;
@property(readwrite, nonatomic, strong) NSTimer *update_file_status_timer_;
@property(readwrite, nonatomic, strong) dispatch_queue_t client_command_queue_;
@end

static const char *const kClientCommandQueueName =
    "com.seafile.seafile-client.findersync.ClientCommandQueue";
static const NSArray *const kBadgetIdentifiers = @[
    // According to the document
    // https://developer.apple.com/library/mac/documentation/FinderSync/Reference/FIFinderSyncController_Class/#//apple_ref/occ/instm/FIFinderSyncController/setBadgeIdentifier:forURL:
    // Setting the identifier to an empty string (@"") removes the badge.
    @"",            // none
    @"syncing",      // syncing
    @"error",        // error
    @"",            // ignored
    @"synced",       // synced
    @"paused",       // readonly
    @"paused",       // paused
    @"locked",       // locked
    @"locked_by_me", // locked by me
];

// Set up images for our badge identifiers. For demonstration purposes,
static void initializeBadgeImages() {
    // Set up images for our badge identifiers. For demonstration purposes,
    // NONE,
    // @""
    // SYNCING,
    [[FIFinderSyncController defaultController]
             setBadgeImage:[NSImage imageNamed:@"status-syncing.icns"]
                     label:NSLocalizedString(@"Syncing", @"Status Syncing")
        forBadgeIdentifier:kBadgetIdentifiers[PathStatus::SYNC_STATUS_SYNCING]];
    // ERROR,
    [[FIFinderSyncController defaultController]
             setBadgeImage:[NSImage imageNamed:@"status-error.icns"]
                     label:NSLocalizedString(@"Error", @"Status Erorr")
        forBadgeIdentifier:kBadgetIdentifiers[PathStatus::SYNC_STATUS_ERROR]];
    // SYNCED,
    [[FIFinderSyncController defaultController]
             setBadgeImage:[NSImage imageNamed:@"status-done.icns"]
                     label:NSLocalizedString(@"Finished", @"Status Finished")
        forBadgeIdentifier:kBadgetIdentifiers[PathStatus::SYNC_STATUS_SYNCED]];
    // PAUSED,
    [[FIFinderSyncController defaultController]
             setBadgeImage:[NSImage imageNamed:@"status-ignored.icns"]
                     label:NSLocalizedString(@"Paused", @"Status Paused")
        forBadgeIdentifier:kBadgetIdentifiers[PathStatus::SYNC_STATUS_PAUSED]];
    // LOCKED,
    [[FIFinderSyncController defaultController]
             setBadgeImage:[NSImage imageNamed:@"status-locked.icns"]
                     label:NSLocalizedString(@"Locked", @"Status Locked")
        forBadgeIdentifier:kBadgetIdentifiers[PathStatus::SYNC_STATUS_LOCKED]];
    // LOCKED_BY_ME,
    [[FIFinderSyncController defaultController]
             setBadgeImage:[NSImage imageNamed:@"status-locked-by-me.icns"]
                     label:NSLocalizedString(@"Locked", @"Status LockedByMe")
        forBadgeIdentifier:kBadgetIdentifiers
                               [PathStatus::SYNC_STATUS_LOCKED_BY_ME]];
}

inline static void setBadgeIdentifierFor(NSURL *url, PathStatus status) {
    [[FIFinderSyncController defaultController]
        setBadgeIdentifier:kBadgetIdentifiers[status]
                    forURL:url];
}

inline static void setBadgeIdentifierFor(const std::string &path,
                                         PathStatus status) {
    bool isDirectory = path.back() == '/';
    std::string file = path;
    if (isDirectory)
        file.resize(file.size() - 1);
    setBadgeIdentifierFor(
        [NSURL fileURLWithPath:[NSString stringWithUTF8String:file.c_str()]
                   isDirectory:isDirectory],
        status);
}

inline static bool isUnderFolderDirectly(const std::string &path,
                                         const std::string &dir) {
    if (strncmp(dir.data(), path.data(), dir.size()) != 0) {
        return false;
    }
    const char *pos = path.data() + dir.size();
    const char *end = pos + path.size() - (dir.size());
    if (end == pos)
        return true;
    // remove the trailing "/" in the end
    if (*(end - 1) == '/')
        --end;
    while (pos != end)
        if (*pos++ == '/')
            return false;
    return true;
}

inline static std::vector<LocalRepo>::const_iterator
findRepo(const std::vector<LocalRepo> &repos, const std::string &repo_id) {
    auto pos = repos.begin();
    for (; pos != repos.end(); ++pos) {
        if (repo_id == pos->repo_id)
            break;
    }
    return pos;
}

inline static std::string getRelativePath(const std::string &path,
                                          const std::string &prefix) {

    std::string relative_path;
    // remove the trailing "/" in the header
    if (path.size() != prefix.size()) {
        relative_path = std::string(path.data() + prefix.size() + 1,
                                    path.size() - prefix.size() - 1);
    }
    return relative_path;
}

inline static bool isContainsPrefix(const std::string &path,
                                    const std::string &prefix) {
    if (prefix.size() > path.size())
        return false;
    if (0 != strncmp(prefix.data(), path.data(), prefix.size()))
        return false;
    if (prefix.size() < path.size() && path[prefix.size()] != '/')
        return false;
    return true;
}

inline static std::vector<LocalRepo>::const_iterator
findRepoContainPath(const std::vector<LocalRepo> &repos,
                    const std::string &path) {
    for (auto repo = repos.begin(); repo != repos.end(); ++repo) {
        if (isContainsPrefix(path, repo->worktree))
            return repo;
    }
    return repos.end();
}

inline static void cleanEntireDirectoryStatus(
    std::unordered_map<std::string, PathStatus> *file_status,
    const std::string &dir) {
    for (auto file = file_status->begin(); file != file_status->end();) {
        auto pos = file++;
        if (!isContainsPrefix(pos->first, dir))
            continue;
        setBadgeIdentifierFor(pos->first, PathStatus::SYNC_STATUS_NONE);
        file_status->erase(pos);
    }
}

inline static void
cleanDirectoryStatus(std::unordered_map<std::string, PathStatus> *file_status,
                     const std::string &dir) {
    for (auto file = file_status->begin(); file != file_status->end();) {
        auto pos = file++;
        if (!isUnderFolderDirectly(pos->first, dir))
            continue;
        file_status->erase(pos);
    }
}

static void
cleanFileStatus(std::unordered_map<std::string, PathStatus> *file_status,
                const std::vector<LocalRepo> &watch_set,
                const std::vector<LocalRepo> &new_watch_set) {
    for (const auto &repo : watch_set) {
        bool found = false;
        for (const auto &new_repo : new_watch_set) {
            if (repo == new_repo) {
                found = true;
                break;
            }
        }
        // cleanup old
        if (!found) {
            // clean up root
            setBadgeIdentifierFor(repo.worktree, PathStatus::SYNC_STATUS_NONE);

            // clean up leafs
            cleanEntireDirectoryStatus(file_status, repo.worktree);
        }
    }
    for (const auto &new_repo : new_watch_set) {
        bool found = false;
        for (const auto &repo : watch_set) {
            if (repo == new_repo) {
                found = true;
                break;
            }
        }
        // add new if necessary
        if (!found)
            file_status->emplace(new_repo.worktree + "/",
                                 PathStatus::SYNC_STATUS_NONE);
    }
}

static std::vector<LocalRepo> watched_repos_;
static std::unordered_map<std::string, PathStatus> file_status_;
static FinderSyncClient *client_ = nullptr;
static constexpr double kGetWatchSetInterval = 5.0;   // seconds
static constexpr double kGetFileStatusInterval = 2.0; // seconds

@implementation FinderSync

- (instancetype)init {
    self = [super init];

#ifdef NDEBUG
    NSLog(@"%s launched from %@ ; compiled at %s", __PRETTY_FUNCTION__,
          [[NSBundle mainBundle] bundlePath], __DATE__);
#else
    NSLog(@"%s launched from %@ ; compiled at %s %s", __PRETTY_FUNCTION__,
          [[NSBundle mainBundle] bundlePath], __TIME__, __DATE__);
#endif

    // Set up client queue
    self.client_command_queue_ =
        dispatch_queue_create(kClientCommandQueueName, DISPATCH_QUEUE_SERIAL);
    // Set up client
    client_ = new FinderSyncClient(self);
    self.update_watch_set_timer_ =
        [NSTimer scheduledTimerWithTimeInterval:kGetWatchSetInterval
                                         target:self
                                       selector:@selector(requestUpdateWatchSet)
                                       userInfo:nil
                                        repeats:YES];

    self.update_file_status_timer_ = [NSTimer
        scheduledTimerWithTimeInterval:kGetFileStatusInterval
                                target:self
                              selector:@selector(requestUpdateFileStatus)
                              userInfo:nil
                               repeats:YES];

    [FIFinderSyncController defaultController].directoryURLs = nil;

    return self;
}

- (void)dealloc {
    delete client_;
    self.client_command_queue_ = nil;
}

#pragma mark - Primary Finder Sync protocol methods

- (void)beginObservingDirectoryAtURL:(NSURL *)url {
    // convert NFD to NFC
    std::string absolute_path =
        url.path.precomposedStringWithCanonicalMapping.UTF8String;

    // find where we have it
    auto repo = findRepoContainPath(watched_repos_, absolute_path);
    if (repo == watched_repos_.end())
        return;

    if (absolute_path.back() != '/')
        absolute_path += "/";

    file_status_.emplace(absolute_path, PathStatus::SYNC_STATUS_NONE);
}

- (void)endObservingDirectoryAtURL:(NSURL *)url {
    // convert NFD to NFC
    std::string absolute_path =
        url.path.precomposedStringWithCanonicalMapping.UTF8String;

    if (absolute_path.back() != '/')
        absolute_path += "/";

    cleanDirectoryStatus(&file_status_, absolute_path);
}

- (void)requestBadgeIdentifierForURL:(NSURL *)url {
    // convert NFD to NFC
    std::string file_path =
        url.path.precomposedStringWithCanonicalMapping.UTF8String;

    // find where we have it
    auto repo = findRepoContainPath(watched_repos_, file_path);
    if (repo == watched_repos_.end())
        return;

    NSNumber *isDirectory;
    if ([url getResourceValue:&isDirectory
                       forKey:NSURLIsDirectoryKey
                        error:nil] &&
        [isDirectory boolValue]) {
        file_path += "/";
    }

    std::string repo_id = repo->repo_id;
    std::string relative_path = getRelativePath(file_path, repo->worktree);
    if (relative_path.empty())
        return;

    file_status_.emplace(file_path, PathStatus::SYNC_STATUS_NONE);
    setBadgeIdentifierFor(file_path, PathStatus::SYNC_STATUS_NONE);
    dispatch_async(self.client_command_queue_, ^{
      client_->doGetFileStatus(repo_id.c_str(), relative_path.c_str());
    });
}

#pragma mark - Menu and toolbar item support

#if 0
- (NSString *)toolbarItemName {
  return @"Seafile FinderSync";
}

- (NSString *)toolbarItemToolTip {
  return @"Seafile FinderSync: Click the toolbar item for a menu.";
}

- (NSImage *)toolbarItemImage {
  return [NSImage imageNamed:NSImageNameFolder];
}
#endif

- (NSMenu *)menuForMenuKind:(FIMenuKind)whichMenu {
    if (whichMenu != FIMenuKindContextualMenuForItems &&
        whichMenu != FIMenuKindContextualMenuForContainer)
        return nil;

    // Produce a menu for the extension.
    NSMenu *menu = [[NSMenu alloc] initWithTitle:@""];
    NSMenuItem *shareLinkItem =
        [menu addItemWithTitle:NSLocalizedString(@"Get Seafile Download Link",
                                                 @"Get Seafile Download Link")
                        action:@selector(shareLinkAction:)
                 keyEquivalent:@""];
    NSImage *seafileImage = [NSImage imageNamed:@"seafile.icns"];
    [shareLinkItem setImage:seafileImage];

    NSArray *items =
        [[FIFinderSyncController defaultController] selectedItemURLs];
    if (![items count])
        return nil;
    NSURL *item = items.firstObject;
    std::string file_path =
        item.path.precomposedStringWithCanonicalMapping.UTF8String;

    auto repo = findRepoContainPath(watched_repos_, file_path);
    if (repo != watched_repos_.end() && repo->internal_link_supported) {
        NSMenuItem *internalLinkItem =
            [menu addItemWithTitle:NSLocalizedString(@"Get Seafile Internal Link",
                                                     @"Get Seafile Internal Link")
                            action:@selector(internalLinkAction:)
                     keyEquivalent:@""];
        [internalLinkItem setImage:seafileImage];
    }

    // add a menu item for lockFile

    NSNumber *isDirectory;
    bool is_dir = [item getResourceValue:&isDirectory
                                  forKey:NSURLIsDirectoryKey
                                   error:nil] &&
                  [isDirectory boolValue];

    // we don't have a lock-file menuitem for folders
    // early return
    if (is_dir)
        return menu;

    // find where we have it
    auto file = file_status_.find(is_dir ? file_path + "/" : file_path);
    if (file != file_status_.end()) {
        NSString *lockFileTitle;
        if (file->second != PathStatus::SYNC_STATUS_LOCKED) {
            if (file->second == PathStatus::SYNC_STATUS_LOCKED_BY_ME) {
                lockFileTitle =
                    NSLocalizedString(@"Unlock This File", @"Unlock This File");
            } else {
                lockFileTitle = NSLocalizedString(@"Lock This File", @"Lock This File");
            }
        } else {
            lockFileTitle = NSLocalizedString(@"Lock This File", @"Lock This File");
        }

        NSMenuItem *lockFileItem = [menu addItemWithTitle:lockFileTitle
                                                   action:@selector(lockFileAction:)
                                            keyEquivalent:@""];

        [lockFileItem setImage:seafileImage];

        if (file->second == PathStatus::SYNC_STATUS_LOCKED)
            [lockFileItem setEnabled:FALSE];
    }

    // NSString *showHistoryTitle;
    NSMenuItem *showHistoryItem =
        [menu addItemWithTitle:NSLocalizedString(@"View File History",
                                                 @"View File History")
                        action:@selector(showHistoryAction:)
                 keyEquivalent:@""];
    [showHistoryItem setImage:seafileImage];

    return menu;
}

- (IBAction)shareLinkAction:(id)sender {
    NSArray *items =
        [[FIFinderSyncController defaultController] selectedItemURLs];
    if (![items count])
        return;
    NSURL *item = items.firstObject;

    // do it in another thread
    std::string path =
        item.path.precomposedStringWithCanonicalMapping.UTF8String;
    NSNumber *isDirectory;
    if ([item getResourceValue:&isDirectory
                        forKey:NSURLIsDirectoryKey
                         error:nil] &&
        [isDirectory boolValue])
        path += "/";

    dispatch_async(self.client_command_queue_, ^{
      client_->doSendCommandWithPath(FinderSyncClient::DoShareLink,
                                     path.c_str());
    });
}

- (IBAction)internalLinkAction:(id)sender {
    NSArray *items =
        [[FIFinderSyncController defaultController] selectedItemURLs];
    if (![items count])
        return;
    NSURL *item = items.firstObject;

    // do it in another thread
    std::string path =
        item.path.precomposedStringWithCanonicalMapping.UTF8String;
    NSNumber *isDirectory;
    if ([item getResourceValue:&isDirectory
                        forKey:NSURLIsDirectoryKey
                         error:nil] &&
        [isDirectory boolValue])
        path += "/";

    dispatch_async(self.client_command_queue_, ^{
      client_->doSendCommandWithPath(FinderSyncClient::DoInternalLink,
                                     path.c_str());
    });
}

- (IBAction)lockFileAction:(id)sender {
    NSArray *items =
        [[FIFinderSyncController defaultController] selectedItemURLs];
    if (![items count])
        return;
    NSURL *item = items.firstObject;

    std::string path =
        item.path.precomposedStringWithCanonicalMapping.UTF8String;
    // find where we have it
    auto file = file_status_.find(path);
    if (file == file_status_.end())
        return;
    if (file->second == PathStatus::SYNC_STATUS_LOCKED)
        return;

    FinderSyncClient::CommandType command;
    if (file->second == PathStatus::SYNC_STATUS_LOCKED_BY_ME)
        command = FinderSyncClient::DoUnlockFile;
    else
        command = FinderSyncClient::DoLockFile;

    // we cannot lock a directory
    NSNumber *isDirectory;
    if ([item getResourceValue:&isDirectory
                        forKey:NSURLIsDirectoryKey
                         error:nil] &&
        [isDirectory boolValue])
        return;

    // do it in another thread
    dispatch_async(self.client_command_queue_, ^{
      client_->doSendCommandWithPath(command, path.c_str());
    });
}

- (IBAction)showHistoryAction:(id)sender {
    NSArray *items =
        [[FIFinderSyncController defaultController] selectedItemURLs];
    if (![items count])
        return;
    NSURL *item = items.firstObject;

    // do it in another thread
    std::string path =
        item.path.precomposedStringWithCanonicalMapping.UTF8String;
    dispatch_async(self.client_command_queue_, ^{
      client_->doSendCommandWithPath(FinderSyncClient::DoShowFileHistory,
                                     path.c_str());
    });
}


- (void)requestUpdateWatchSet {
    // do it in another thread
    dispatch_async(self.client_command_queue_, ^{
      client_->getWatchSet();
    });
}

- (void)updateWatchSet:(void *)ptr_to_new_watched_repos {
    std::vector<LocalRepo> new_watched_repos;
    if (ptr_to_new_watched_repos)
        new_watched_repos = std::move(
            *static_cast<std::vector<LocalRepo> *>(ptr_to_new_watched_repos));

    cleanFileStatus(&file_status_, watched_repos_, new_watched_repos);

    // overwrite the old watch set
    watched_repos_ = std::move(new_watched_repos);

    // update FIFinderSyncController's directory URLs
    NSMutableArray *array =
        [NSMutableArray arrayWithCapacity:watched_repos_.size()];
    for (const LocalRepo &repo : watched_repos_) {
        NSString *path = [NSString stringWithUTF8String:repo.worktree.c_str()];
        [array addObject:[NSURL fileURLWithPath:path isDirectory:YES]];
    }

    [FIFinderSyncController defaultController].directoryURLs =
        [NSSet setWithArray:array];

    // initialize the badge images
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        initializeBadgeImages();
    }
}

- (void)requestUpdateFileStatus {
    for (const auto &pair : file_status_) {
        auto repo = findRepoContainPath(watched_repos_, pair.first);
        if (repo == watched_repos_.end()) /* erase it ?*/
            continue;

        std::string relative_path = getRelativePath(pair.first, repo->worktree);
        if (relative_path.empty())
            relative_path = "/";
        std::string repo_id = repo->repo_id;
        dispatch_async(self.client_command_queue_, ^{
          client_->doGetFileStatus(repo_id.c_str(), relative_path.c_str());
        });
    }
}

- (void)updateFileStatus:(const char *)repo_id
                    path:(const char *)path
                  status:(uint32_t)status {
    auto repo = findRepo(watched_repos_, repo_id);
    if (repo == watched_repos_.end())
        return;

    std::string absolute_path =
        repo->worktree + (*path == '/' ? "" : "/") + path;

    // if the path is erased, ignore it!
    auto file = file_status_.find(absolute_path);
    if (file == file_status_.end())
        return;

    // always set up, avoid some bugs
    file->second = static_cast<PathStatus>(status);
    setBadgeIdentifierFor(absolute_path, static_cast<PathStatus>(status));
}

@end
