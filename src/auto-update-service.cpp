#include <QSettings>

#ifdef Q_OS_WIN32
    #include <winsparkle.h>
#else
    #include "mac-sparkle-support.h"
#endif

#include "api/requests.h"
#include "seafile-applet.h"
#include "utils/utils.h"

#include "auto-update-service.h"

SINGLETON_IMPL(AutoUpdateService)

namespace
{
#ifdef Q_OS_WIN32
    const char *kSparkleAppcastURI = "https://seafile.com/api/client-updates/seafile-client-windows/appcast.xml";
    const char *kWinSparkleRegistryPath = "SOFTWARE\\Seafile\\Seafile Client\\WinSparkle";
#else
    const char *kSparkleAppcastURI = "https://seafile.com/api/client-updates/seafile-client-mac/appcast.xml";
#endif
    const char *kConfirmSparkleCheckUpdate = "ConfirmWinsparkCheckUpdate";

QString getAppcastURI() {
    QString url_from_env = qgetenv("SEAFILE_CLIENT_APPCAST_URI");
    if (!url_from_env.isEmpty()) {
        qWarning(
            "winsparkle: using app cast url from SEAFILE_CLIENT_APPCAST_URI: "
            "%s",
            url_from_env.toUtf8().data());
        return url_from_env;
    }
    return kSparkleAppcastURI;
}

} // namespace

// Virtual base class for windows/mac
class AutoUpdateAdapter {
public:
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void checkNow() = 0;
    virtual bool autoUpdateEnabled() = 0;
    virtual void setAutoUpdateEnabled(bool enabled) = 0;
};

#ifdef Q_OS_WIN32
class WindowsAutoUpdateAdapter: public AutoUpdateAdapter {
public:
    void start() {
        prepare();
        win_sparkle_init();
    }

    void stop() {
        win_sparkle_cleanup();
    }

    void checkNow() {
        win_sparkle_check_update_with_ui();
    }

    bool autoUpdateEnabled() {
        // qWarning() << "autoUpdateEnabled =" << win_sparkle_get_automatic_check_for_updates();
        win_sparkle_get_automatic_check_for_updates();
    }

    void setAutoUpdateEnabled(bool enabled) {
        win_sparkle_set_automatic_check_for_updates(enabled ? 1 : 0);
    }

    void prepare() {
        // Note that @param path is relative to HKCU/HKLM root
        // and the root is not part of it. For example:
        // @code
        //     win_sparkle_set_registry_path("Software\\My App\\Updates");
        // @endcode
        win_sparkle_set_registry_path(kWinSparkleRegistryPath);
        win_sparkle_set_appcast_url(getAppcastURI().toUtf8().data());
        win_sparkle_set_app_details(
            L"Seafile",
            L"Seafile Client",
            QString(STRINGIZE(SEAFILE_CLIENT_VERSION)).toStdWString().c_str());

        // Avoid winsparkle to pop up a dialog asking the user "do you want to check
        // for updates automatically?".
        QSettings settings;
        settings.beginGroup("Misc");
        bool confirm_winspark_check_update = settings.value(kConfirmSparkleCheckUpdate, false).toBool();

        if (!confirm_winspark_check_update) {
            settings.setValue(kConfirmSparkleCheckUpdate, true);
            setAutoUpdateEnabled(true);
        }

        settings.endGroup();
    }
};
#elif defined(Q_OS_MAC)
class MacAutoUpdateAdapter: public AutoUpdateAdapter {
public:
    void start() {
        SparkleHelper::setFeedURL(getAppcastURI().toUtf8().data());
    }

    void stop() {
    }

    void checkNow() {
        SparkleHelper::checkForUpdate();
    }

    bool autoUpdateEnabled() {
        return SparkleHelper::autoUpdateEnabled();
    }

    void setAutoUpdateEnabled(bool enabled) {
        SparkleHelper::setAutoUpdateEnabled(enabled);
    }

    void prepare() {
        // Avoid winsparkle to pop up a dialog asking the user "do you want to check
        // for updates automatically?".
        QSettings settings;
        settings.beginGroup("Misc");
        bool confirm_winspark_check_update = settings.value(kConfirmSparkleCheckUpdate, false).toBool();

        if (!confirm_winspark_check_update) {
            settings.setValue(kConfirmSparkleCheckUpdate, true);
            setAutoUpdateEnabled(true);
        }

        settings.endGroup();
    }
};
#endif


AutoUpdateService::AutoUpdateService(QObject *parent) : QObject(parent)
{
#ifdef Q_OS_WIN32
    adapter_ = new WindowsAutoUpdateAdapter;
#else
    adapter_ = new MacAutoUpdateAdapter;
#endif
}

void AutoUpdateService::start()
{
    adapter_->start();
}

void AutoUpdateService::stop()
{
    adapter_->stop();
}


void AutoUpdateService::checkUpdate()
{
    adapter_->checkNow();
}


bool AutoUpdateService::shouldSupportAutoUpdate() const {
    // qWarning() << "shouldSupportAutoUpdate =" << (QString(getBrand()) == "Seafile");
    return QString(getBrand()) == "Seafile";
}

bool AutoUpdateService::autoUpdateEnabled() const {
    return adapter_->autoUpdateEnabled();
}

void AutoUpdateService::setAutoUpdateEnabled(bool enabled) {
    // qWarning() << "setAutoUpdateEnabled:" << enabled;
    adapter_->setAutoUpdateEnabled(enabled);
}
