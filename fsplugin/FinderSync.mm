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
#include <algorithm>

#if !__has_feature(objc_arc)
#error this file must be built with ARC support
#endif

@interface FinderSync ()

@property(readwrite, nonatomic, strong) NSTimer *update_watch_set_timer_;
@end

static std::vector<LocalRepo> watched_repos_;
static std::map<std::string, std::vector<NSURL *>> mapped_observed_files_;
static FinderSyncClient *client_ = nullptr;
static constexpr double kGetWatchSetInterval = 5.0; // seconds
static const NSArray *badgeIdentifiers = @[
    @"DISABLED",
    @"WAITING",
    @"INIT",
    @"ING",
    @"DONE",
    @"ERROR",
    @"UNKNOWN",
    @"UNSET",
];

// Set up images for our badge identifiers. For demonstration purposes,
static void initializeBadgeImages() {
    // Set up images for our badge identifiers. For demonstration purposes,
    // this
    // uses off-the-shelf images.
    // DISABLED
    [[FIFinderSyncController defaultController]
             setBadgeImage:[NSImage imageNamed:@"status-unknown.icns"]
                     label:@"Status Disabled"
        forBadgeIdentifier:badgeIdentifiers[LocalRepo::SYNC_STATE_DISABLED]];
    // WAITING,
    [[FIFinderSyncController defaultController]
             setBadgeImage:[NSImage imageNamed:@"status-done.icns"]
                     label:@"Status Waiting"
        forBadgeIdentifier:badgeIdentifiers[LocalRepo::SYNC_STATE_WAITING]];
    // INIT,
    [[FIFinderSyncController defaultController]
             setBadgeImage:[NSImage imageNamed:@"status-done.icns"]
                     label:@"Status Init"
        forBadgeIdentifier:badgeIdentifiers[LocalRepo::SYNC_STATE_INIT]];
    // ING,
    [[FIFinderSyncController defaultController]
             setBadgeImage:[NSImage imageNamed:@"status-syncing.icns"]
                     label:@"Status Ing"
        forBadgeIdentifier:badgeIdentifiers[LocalRepo::SYNC_STATE_ING]];
    // DONE,
    [[FIFinderSyncController defaultController]
             setBadgeImage:[NSImage imageNamed:@"status-done.icns"]
                     label:@"Status Done"
        forBadgeIdentifier:badgeIdentifiers[LocalRepo::SYNC_STATE_DONE]];
    // ERROR,
    [[FIFinderSyncController defaultController]
             setBadgeImage:[NSImage imageNamed:@"status-error.icns"]
                     label:@"Status Error"
        forBadgeIdentifier:badgeIdentifiers[LocalRepo::SYNC_STATE_ERROR]];
    // UNKNOWN,
    [[FIFinderSyncController defaultController]
             setBadgeImage:[NSImage imageNamed:@"status-unknown.icns"]
                     label:@"Status Unknown"
        forBadgeIdentifier:badgeIdentifiers[LocalRepo::SYNC_STATE_UNKNOWN]];
    // UNSET,
    [[FIFinderSyncController defaultController]
             setBadgeImage:[NSImage imageNamed:@""]
                     label:@"Status Unknown"
        forBadgeIdentifier:badgeIdentifiers[LocalRepo::SYNC_STATE_UNSET]];
}

inline static void setBadgeIdentifierFor(NSURL *url,
                                         LocalRepo::SyncState status) {
    [[FIFinderSyncController defaultController]
        setBadgeIdentifier:badgeIdentifiers[status]
                    forURL:url];
}

inline static void setBadgeIdentifierFor(const std::string &path,
                                         LocalRepo::SyncState status) {
    setBadgeIdentifierFor(
        [NSURL fileURLWithFileSystemRepresentation:path.c_str()
                                       isDirectory:YES
                                     relativeToURL:nil],
        status);
}

inline static std::vector<LocalRepo>::const_iterator
findRepo(const std::vector<LocalRepo> &repos, const LocalRepo &repo) {
    auto pos = repos.begin();
    for (; pos != repos.end(); ++pos) {
        if (repo == *pos)
            break;
    }
    return pos;
}

inline static std::vector<LocalRepo>::const_iterator
findRepoContainPath(const std::vector<LocalRepo> &repos,
                    const std::string &path) {
    auto pos = repos.begin();
    for (; pos != repos.end(); ++pos) {
        if (0 ==
            strncmp(pos->worktree.data(), path.data(), pos->worktree.size()))
            break;
    }
    return pos;
}

@implementation FinderSync

- (instancetype)init {
    self = [super init];

    NSLog(@"%s launched from %@ ; compiled at %s", __PRETTY_FUNCTION__,
          [[NSBundle mainBundle] bundlePath], __TIME__);

    // Set up client
    client_ = new FinderSyncClient(self);
    self.update_watch_set_timer_ =
        [NSTimer scheduledTimerWithTimeInterval:kGetWatchSetInterval
                                         target:self
                                       selector:@selector(requestUpdateWatchSet)
                                       userInfo:nil
                                        repeats:YES];

    [FIFinderSyncController defaultController].directoryURLs = nil;

    return self;
}

- (void)dealloc {
    delete client_;
    NSLog(@"%s unloaded ; compiled at %s", __PRETTY_FUNCTION__, __TIME__);
}

#pragma mark - Primary Finder Sync protocol methods

- (void)beginObservingDirectoryAtURL:(NSURL *)url {
    std::string file_path = url.fileSystemRepresentation;

    // find the worktree dir in local repos
    auto pos_of_repo = findRepoContainPath(watched_repos_, file_path);
    if (pos_of_repo == watched_repos_.end())
        return;

    // insert the node based on worktree dir
    auto pos_of_observed_file =
        mapped_observed_files_.find(pos_of_repo->worktree);
    if (pos_of_observed_file == mapped_observed_files_.end())
        mapped_observed_files_.emplace(std::move(file_path),
                                       std::vector<NSURL *>());
}

- (void)endObservingDirectoryAtURL:(NSURL *)url {
    std::string file_path = url.fileSystemRepresentation;

    // find the repo in watch repos
    auto pos_of_repo = findRepoContainPath(watched_repos_, file_path);
    if (pos_of_repo == watched_repos_.end())
        return;

    // find the items in observed items
    auto pos_of_observed_file =
        mapped_observed_files_.find(pos_of_repo->worktree);
    if (pos_of_observed_file == mapped_observed_files_.end())
        return;

    // only remove the items under the exact current dir
    auto &filelist = pos_of_observed_file->second;
    filelist.erase(
        std::remove_if(filelist.begin(), filelist.end(), [&](NSURL *fileurl) {
            return [url isEqual:[fileurl URLByDeletingLastPathComponent]];
        }), filelist.end());
}

- (void)requestBadgeIdentifierForURL:(NSURL *)url {
    std::string file_path = url.fileSystemRepresentation;

    // find where we have it
    auto pos_of_repo = findRepoContainPath(watched_repos_, file_path);
    if (pos_of_repo == watched_repos_.end())
        return;

    // find where we have it
    auto pos_of_observed_file = std::find_if(
        mapped_observed_files_.begin(), mapped_observed_files_.end(),
        [&](decltype(mapped_observed_files_)::value_type &observed_file) {
            return 0 == strncmp(observed_file.first.data(), file_path.data(),
                                observed_file.first.size());

        });
    if (pos_of_observed_file == mapped_observed_files_.end())
        return;

    // TODO remove this to update items in the worktree
    if (file_path != pos_of_repo->worktree)
        return;

    // insert it into observed file list
    pos_of_observed_file->second.push_back(url);

    // set badge image
    setBadgeIdentifierFor(url, pos_of_repo->status);
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
    // Produce a menu for the extension.
    NSMenu *menu = [[NSMenu alloc] initWithTitle:@""];
    NSMenuItem *shareLinkItem =
        [menu addItemWithTitle:@"Get Seafile Share Link"
                        action:@selector(shareLinkAction:)
                 keyEquivalent:@""];
    NSImage *seafileImage = [NSImage imageNamed:@"seafile.icns"];
    [shareLinkItem setImage:seafileImage];

    return menu;
}

- (IBAction)shareLinkAction:(id)sender {
    NSArray *items =
        [[FIFinderSyncController defaultController] selectedItemURLs];
    if (![items count])
        return;
    NSURL *item = items.firstObject;

    std::string fileName = [item fileSystemRepresentation];

    // do it in another thread
    dispatch_async(
        dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0), ^{
          client_->doSharedLink(fileName.c_str());
        });
}

- (void)requestUpdateWatchSet {
    // do it in another thread
    dispatch_async(
        dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0), ^{
          client_->getWatchSet();
        });
}

- (void)updateWatchSet:(void *)ptr_to_new_watched_repos {
    std::vector<LocalRepo> new_watched_repos;
    if (ptr_to_new_watched_repos)
        new_watched_repos = std::move(
            *static_cast<std::vector<LocalRepo> *>(ptr_to_new_watched_repos));

    // unset outdate repos
    // (partly done by endObservingDirectoryAtURL, but it is not sufficient)
    //
    for (auto &repo : watched_repos_) {
        // if confilcting, we don't need to unset it
        if (new_watched_repos.end() != findRepo(new_watched_repos, repo))
            continue;

        // set up for the root of worktree
        setBadgeIdentifierFor(repo.worktree, LocalRepo::SYNC_STATE_UNSET);

        // update the belonging observed items if any
        auto pos_of_observed_file = mapped_observed_files_.find(repo.worktree);

        // skip if not items observed
        if (pos_of_observed_file == mapped_observed_files_.end())
            continue;

        // update old files status
        for (NSURL *url : pos_of_observed_file->second)
            setBadgeIdentifierFor(url, LocalRepo::SYNC_STATE_UNSET);
    }

    // update all dirs and files in new repos
    for (auto &new_repo : new_watched_repos) {
        // if not conflicting repo, namely, newly-added repo
        auto pos_of_old_repo = findRepo(watched_repos_, new_repo);
        if (watched_repos_.end() == pos_of_old_repo) {
            // set up for the root of worktree
            setBadgeIdentifierFor(new_repo.worktree, new_repo.status);
            continue;
        }

        // update for the root of worktree
        setBadgeIdentifierFor(new_repo.worktree, new_repo.status);

        // skip if not items observed
        auto pos_of_observed_file =
            mapped_observed_files_.find(new_repo.worktree);

        // update only the ones which status has been changed
        if (pos_of_observed_file == mapped_observed_files_.end() ||
            pos_of_old_repo->status == new_repo.status)
            continue;

        LocalRepo::SyncState new_status = new_repo.status;

        // update old files status
        for (NSURL *url : pos_of_observed_file->second)
            setBadgeIdentifierFor(url, new_status);
    }

    // overwrite the old watch set
    watched_repos_ = std::move(new_watched_repos);

    // update FIFinderSyncController's directory URLs
    NSMutableArray *array =
        [NSMutableArray arrayWithCapacity:watched_repos_.size()];
    for (const LocalRepo &repo : watched_repos_) {
        [array
            addObject:[NSURL fileURLWithFileSystemRepresentation:repo.worktree
                                                                     .c_str()
                                                     isDirectory:YES
                                                   relativeToURL:nil]];
    }

    [FIFinderSyncController defaultController].directoryURLs =
        [NSSet setWithArray:array];

    // initialize the badge images
    static BOOL initialized = FALSE;
    if (!initialized) {
        initialized = TRUE;
        initializeBadgeImages();
    }
}

@end
