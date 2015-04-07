#include "sparkle-support.h"

#import "SUUpdater.h"

void SparkleHelper::checkForUpdate(bool force)
{
    if (force || [[SUUpdater sharedUpdater] automaticallyChecksForUpdates])
        [[SUUpdater sharedUpdater] checkForUpdatesInBackground];
}

void SparkleHelper::setAutomaticallyChecksForUpdates(bool enabled)
{
    [[SUUpdater sharedUpdater] setAutomaticallyChecksForUpdates: enabled];
    [[SUUpdater sharedUpdater] setAutomaticallyDownloadsUpdates: enabled];
}
