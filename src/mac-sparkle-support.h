#ifndef SEAFILE_CLIENT_SPARKLE_SUPPORT_H
#define SEAFILE_CLIENT_SPARKLE_SUPPORT_H

class SparkleHelper {
public:
    static void checkForUpdate();
    static void setAutoUpdateEnabled(bool enabled);
    static bool autoUpdateEnabled();
    static void setFeedURL(const char* url);
};

#endif // SEAFILE_CLIENT_SPARKLE_SUPPORT_H
