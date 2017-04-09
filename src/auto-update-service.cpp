#include <QSettings>

#include <winsparkle.h>

#include "api/requests.h"
#include "seafile-applet.h"
#include "utils/utils.h"

#include "auto-update-service.h"

SINGLETON_IMPL(AutoUpdateService)

namespace
{
    const char *kSparkleAppcastURI = "https://seafile.com/api/seafile-client/appcast.xml";
    const char *kWinSparkleRegistryPath = "SOFTWARE\\Seafile\\Seafile Client\\WinSparkle";
    const char *kConfirmSparkleCheckUpdate = "ConfirmWinsparkCheckUpdate";
} // namespace

AutoUpdateService::AutoUpdateService(QObject *parent) : QObject(parent)
{
}

void AutoUpdateService::start()
{
    // Initialize the updater and possibly show some UI
    win_sparkle_init();
}

void AutoUpdateService::stop()
{
    win_sparkle_cleanup();
}


void AutoUpdateService::checkUpdate()
{
    win_sparkle_check_update_with_ui();
}

void AutoUpdateService::checkAndInstallUpdate() {
    win_sparkle_check_update_with_ui_and_install();
}

void AutoUpdateService::checkUpdateWithoutUI() {
    win_sparkle_check_update_without_ui();
}


void AutoUpdateService::setRequestParams() {
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

bool AutoUpdateService::shouldSupportAutoUpdate() const {
    // qWarning() << "shouldSupportAutoUpdate =" << (QString(getBrand()) == "Seafile");
    return QString(getBrand()) == "Seafile";
}

bool AutoUpdateService::autoUpdateEnabled() const {
    // qWarning() << "autoUpdateEnabled =" << win_sparkle_get_automatic_check_for_updates();
    return win_sparkle_get_automatic_check_for_updates();
}

void AutoUpdateService::setAutoUpdateEnabled(bool enabled) {
    // qWarning() << "setAutoUpdateEnabled:" << enabled;
    win_sparkle_set_automatic_check_for_updates(enabled ? 1 : 0);
}

uint AutoUpdateService::updateCheckInterval() const {
    return win_sparkle_get_update_check_interval();
}

void AutoUpdateService::setUpdateCheckInterval(uint interval_in_seconds) {
    win_sparkle_set_update_check_interval(interval_in_seconds);
}

QString AutoUpdateService::getAppcastURI() {
#if defined(WINSPARKLE_DEBUG)
    QString url_from_env = qgetenv("SEAFILE_CLIENT_APPCAST_URI");
    if (!url_from_env.isEmpty()) {
        qWarning(
            "winsparkle: using app cast url from SEAFILE_CLIENT_APPCAST_URI: "
            "%s",
            url_from_env.toUtf8().data());
        return url_from_env;
    }
#endif
    return kSparkleAppcastURI;
}
