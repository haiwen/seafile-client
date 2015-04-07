#ifndef SEAFILE_CLIENT_SPARKLE_SUPPORT_H
#define SEAFILE_CLIENT_SPARKLE_SUPPORT_H

class SparkleHelper {
public:
    static void checkForUpdate(bool force = false);
    static void setAutomaticallyChecksForUpdates(bool enabled);
};

#endif // SEAFILE_CLIENT_SPARKLE_SUPPORT_H
