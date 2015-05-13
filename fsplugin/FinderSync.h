//
//  FinderSync.h
//  seafile-client-fsplugin
//
//  Created by Chilledheart on 1/10/15.
//  Copyright (c) 2015 Haiwen. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <FinderSync/FinderSync.h>

@interface FinderSync : FIFinderSync
- (void)updateWatchSet:(void *)ptr_to_new_repos;
- (void)updateFileStatus:(const char *)repo_id
                    path:(const char *)path
                  status:(uint32_t)status;
@end
