#include <QtGlobal>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QtWidgets>
#else
#include <QtGui>
#endif
#include <QDebug>

#include "i18n.h"
#include "account-mgr.h"
#include "utils/utils.h"
#include "seafile-applet.h"
#include "settings-mgr.h"
#include "api/requests.h"
#include "settings-dialog.h"

namespace {

bool isCheckLatestVersionEnabled() {
    return QString(getBrand()) == "Seafile";
}

} // namespace

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent)
{
    setupUi(this);
    setWindowTitle(tr("Settings"));
    setWindowIcon(QIcon(":/images/seafile.png"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    mTabWidget->setCurrentIndex(0);

    // Since closeEvent() would not not called when accept() is called, we
    // need to handle it here.
    connect(this, SIGNAL(accepted()), this, SLOT(updateSettings()));

    if (!isCheckLatestVersionEnabled()) {
        mCheckLatestVersionBox->setVisible(false);
    }

    mLanguageComboBox->addItems(I18NHelper::getInstance()->getLanguages());
    mProxyMethodComboBox->insertItem(SettingsManager::NoneProxy, tr("None"));
    mProxyMethodComboBox->insertItem(SettingsManager::HttpProxy, tr("HTTP Proxy"));
    mProxyMethodComboBox->insertItem(SettingsManager::SocksProxy, tr("Socks5 Proxy"));
    connect(mProxyMethodComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(proxyMethodChanged(int)));
    connect(mProxyRequirePassword, SIGNAL(stateChanged(int)),
            this, SLOT(proxyRequirePasswordChanged(int)));

    #if defined(Q_OS_MAC)
    layout()->setContentsMargins(8, 9, 9, 4);
    layout()->setSpacing(5);
    #endif
}

void SettingsDialog::updateSettings()
{
    SettingsManager *mgr = seafApplet->settingsManager();
    mgr->setNotify(mNotifyCheckBox->checkState() == Qt::Checked);
    mgr->setAutoStart(mAutoStartCheckBox->checkState() == Qt::Checked);
    mgr->setHideDockIcon(mHideDockIconCheckBox->checkState() == Qt::Checked);
    mgr->setSyncExtraTempFile(mSyncExtraTempFileCheckBox->checkState() == Qt::Checked);
    mgr->setMaxDownloadRatio(mDownloadSpinBox->value());
    mgr->setMaxUploadRatio(mUploadSpinBox->value());
    mgr->setHideMainWindowWhenStarted(mHideMainWinCheckBox->checkState() == Qt::Checked);
    mgr->setAllowInvalidWorktree(mAllowInvalidWorktreeCheckBox->checkState() == Qt::Checked);
    mgr->setHttpSyncEnabled(mEnableHttpSyncCheckBox->checkState() == Qt::Checked);
    mgr->setHttpSyncCertVerifyDisabled(mDisableVerifyHttpSyncCert->checkState() == Qt::Checked);
    mgr->setAllowRepoNotFoundOnServer(mAllowRepoNotFoundCheckBox->checkState() == Qt::Checked);
#ifdef HAVE_FINDER_SYNC_SUPPORT
    if(mFinderSyncCheckBox->isEnabled())
        mgr->setFinderSyncExtension(mFinderSyncCheckBox->checkState() == Qt::Checked);
#endif
#ifdef Q_OS_WIN32
    mgr->setShellExtensionEnabled(mShellExtCheckBox->checkState() == Qt::Checked);
#endif

    bool proxy_changed = updateProxySettings();

    if (isCheckLatestVersionEnabled()) {
        bool enabled = mCheckLatestVersionBox->checkState() == Qt::Checked;
        mgr->setCheckLatestVersionEnabled(enabled);
    }

    bool language_changed = false;
    if (mLanguageComboBox->currentIndex() != I18NHelper::getInstance()->preferredLanguage()) {
        language_changed = true;
        I18NHelper::getInstance()->setPreferredLanguage(mLanguageComboBox->currentIndex());
    }

    if (language_changed && seafApplet->yesOrNoBox(tr("You have changed languange. Restart to apply it?"), this, true))
        seafApplet->restartApp();

    if (proxy_changed && seafApplet->yesOrNoBox(tr("You have changed proxy settings. Restart to apply it?"), this, true))
        seafApplet->restartApp();

}

void SettingsDialog::closeEvent(QCloseEvent *event)
{
    event->ignore();
    this->hide();
}

void SettingsDialog::showEvent(QShowEvent *event)
{
    Qt::CheckState state;
    int ratio;

    SettingsManager *mgr = seafApplet->settingsManager();

    mgr->loadSettings();

    state = mgr->hideMainWindowWhenStarted() ? Qt::Checked : Qt::Unchecked;
    mHideMainWinCheckBox->setCheckState(state);

    state = mgr->allowInvalidWorktree() ? Qt::Checked : Qt::Unchecked;
    mAllowInvalidWorktreeCheckBox->setCheckState(state);

    state = mgr->syncExtraTempFile() ? Qt::Checked : Qt::Unchecked;
    mSyncExtraTempFileCheckBox->setCheckState(state);

    state = mgr->allowRepoNotFoundOnServer() ? Qt::Checked : Qt::Unchecked;
    mAllowRepoNotFoundCheckBox->setCheckState(state);

    state = mgr->httpSyncEnabled() ? Qt::Checked : Qt::Unchecked;
    mEnableHttpSyncCheckBox->setCheckState(state);

    state = mgr->httpSyncCertVerifyDisabled() ? Qt::Checked : Qt::Unchecked;
    mDisableVerifyHttpSyncCert->setCheckState(state);

    // currently supports windows only
    state = mgr->autoStart() ? Qt::Checked : Qt::Unchecked;
    mAutoStartCheckBox->setCheckState(state);
#if !defined(Q_OS_WIN32) && !defined(Q_OS_MAC)
    mAutoStartCheckBox->hide();
#endif
#ifdef HAVE_FINDER_SYNC_SUPPORT
    if (mgr->getFinderSyncExtensionAvailable()) {
        mFinderSyncCheckBox->setEnabled(true);
        state = mgr->getFinderSyncExtension() ? Qt::Checked : Qt::Unchecked;
        mFinderSyncCheckBox->setCheckState(state);
    } else {
        mFinderSyncCheckBox->setEnabled(false);
    }
#else
    mFinderSyncCheckBox->hide();
#endif

#if defined(Q_OS_WIN32)
    state = mgr->shellExtensionEnabled() ? Qt::Checked : Qt::Unchecked;
    mShellExtCheckBox->setCheckState(state);
#endif

    // currently supports mac only
    state = mgr->hideDockIcon() ? Qt::Checked : Qt::Unchecked;
    mHideDockIconCheckBox->setCheckState(state);
#if !defined(Q_OS_MAC)
    mHideDockIconCheckBox->hide();
#endif

    state = mgr->notify() ? Qt::Checked : Qt::Unchecked;
    mNotifyCheckBox->setCheckState(state);

    ratio = mgr->maxDownloadRatio();
    mDownloadSpinBox->setValue(ratio);
    ratio = mgr->maxUploadRatio();
    mUploadSpinBox->setValue(ratio);

    if (isCheckLatestVersionEnabled()) {
        state = mgr->isCheckLatestVersionEnabled() ? Qt::Checked : Qt::Unchecked;
        mCheckLatestVersionBox->setCheckState(state);
    }

    SettingsManager::ProxyType proxy_type;
    QString proxy_host;
    QString proxy_username;
    QString proxy_password;
    int proxy_port;
    mgr->getProxy(proxy_type, proxy_host, proxy_port, proxy_username, proxy_password);
    proxyMethodChanged(proxy_type);
    mProxyMethodComboBox->setCurrentIndex(proxy_type);
    mProxyHost->setText(proxy_host);
    mProxyPort->setValue(proxy_port);
    mProxyUsername->setText(proxy_username);
    mProxyPassword->setText(proxy_password);
    if (!proxy_username.isEmpty())
        mProxyRequirePassword->setChecked(true);

    mLanguageComboBox->setCurrentIndex(I18NHelper::getInstance()->preferredLanguage());

    QDialog::showEvent(event);
}


void SettingsDialog::autoStartChanged(int state)
{
    qDebug("%s :%d", __func__, state);
    bool autoStart = (mAutoStartCheckBox->checkState() == Qt::Checked);
    seafApplet->settingsManager()->setAutoStart(autoStart);
}

void SettingsDialog::hideDockIconChanged(int state)
{
    qDebug("%s :%d", __func__, state);
    bool hideDockIcon = (mHideDockIconCheckBox->checkState() == Qt::Checked);
    seafApplet->settingsManager()->setHideDockIcon(hideDockIcon);
}

void SettingsDialog::notifyChanged(int state)
{
    qDebug("%s :%d", __func__, state);
    bool notify = (mNotifyCheckBox->checkState() == Qt::Checked);
    seafApplet->settingsManager()->setNotify(notify);
}

void SettingsDialog::downloadChanged(int value)
{
    qDebug("%s :%d", __func__, value);
    seafApplet->settingsManager()->setMaxDownloadRatio(mDownloadSpinBox->value());
}

void SettingsDialog::uploadChanged(int value)
{
    qDebug("%s :%d", __func__, value);
    seafApplet->settingsManager()->setMaxUploadRatio(mUploadSpinBox->value());
}

void SettingsDialog::proxyRequirePasswordChanged(int state)
{
    if (state == Qt::Checked) {
        mProxyUsername->setEnabled(true);
        mProxyUsernameLabel->setEnabled(true);
        mProxyPassword->setEnabled(true);
        mProxyPasswordLabel->setEnabled(true);
    } else {
        mProxyUsername->setEnabled(false);
        mProxyUsernameLabel->setEnabled(false);
        mProxyPassword->setEnabled(false);
        mProxyPasswordLabel->setEnabled(false);
    }
}

void SettingsDialog::proxyMethodChanged(int state)
{
    SettingsManager::ProxyType proxy_type =
        static_cast<SettingsManager::ProxyType>(state);
    switch(proxy_type) {
        case SettingsManager::HttpProxy:
            mProxyHost->setVisible(true);
            mProxyHostLabel->setVisible(true);
            mProxyPort->setVisible(true);
            mProxyPortLabel->setVisible(true);
            mProxyRequirePassword->setVisible(true);
            mProxyUsername->setVisible(true);
            mProxyUsernameLabel->setVisible(true);
            mProxyPassword->setVisible(true);
            mProxyPasswordLabel->setVisible(true);
            break;
        case SettingsManager::SocksProxy:
            mProxyHost->setVisible(true);
            mProxyHostLabel->setVisible(true);
            mProxyPort->setVisible(true);
            mProxyPortLabel->setVisible(true);
            mProxyRequirePassword->setVisible(false);
            mProxyUsername->setVisible(false);
            mProxyUsernameLabel->setVisible(false);
            mProxyPassword->setVisible(false);
            mProxyPasswordLabel->setVisible(false);
            break;
        case SettingsManager::NoneProxy:
        default:
            mProxyHost->setVisible(false);
            mProxyHostLabel->setVisible(false);
            mProxyPort->setVisible(false);
            mProxyPortLabel->setVisible(false);
            mProxyRequirePassword->setVisible(false);
            mProxyUsername->setVisible(false);
            mProxyUsernameLabel->setVisible(false);
            mProxyPassword->setVisible(false);
            mProxyPasswordLabel->setVisible(false);
            break;
    }
}

bool SettingsDialog::updateProxySettings()
{
    bool proxy_changed = false;

    SettingsManager *mgr = seafApplet->settingsManager();
    SettingsManager::ProxyType old_proxy_type;
    QString old_proxy_host;
    QString old_proxy_username;
    QString old_proxy_password;
    int old_proxy_port;
    mgr->getProxy(old_proxy_type, old_proxy_host, old_proxy_port, old_proxy_username, old_proxy_password);

    SettingsManager::ProxyType proxy_type = static_cast<SettingsManager::ProxyType>(mProxyMethodComboBox->currentIndex());
    QString proxy_host = mProxyHost->text().trimmed();
    QString proxy_username = mProxyUsername->text().trimmed();
    QString proxy_password = mProxyPassword->text().trimmed();
    int proxy_port = mProxyPort->value();

    switch(proxy_type) {
        case SettingsManager::HttpProxy:
                // if we setup proxy username now and previously
            if (mProxyRequirePassword->checkState() == Qt::Checked) {
                if (proxy_type == old_proxy_type &&
                    proxy_host == old_proxy_host &&
                    proxy_port == old_proxy_port &&
                    proxy_username == old_proxy_username &&
                    proxy_password == old_proxy_password)
                    break;
                proxy_changed = true;
                mgr->setProxy(SettingsManager::HttpProxy,
                              proxy_host, proxy_port,
                              proxy_username, proxy_password);
                break;
            }
            else {
                // and if we don't setup proxy username now and previously
                if (proxy_type == old_proxy_type &&
                    proxy_host == old_proxy_host &&
                    proxy_port == old_proxy_port &&
                    old_proxy_username.isEmpty())
                    break;
                proxy_changed = true;
                mgr->setProxy(SettingsManager::HttpProxy,
                              proxy_host, proxy_port);
            }
            break;
        case SettingsManager::SocksProxy:
            if (proxy_type == old_proxy_type &&
                proxy_host == old_proxy_host &&
                proxy_port == old_proxy_port)
                break;
            proxy_changed = true;
            mgr->setProxy(SettingsManager::SocksProxy,
                          proxy_host, proxy_port);
            break;
        case SettingsManager::NoneProxy:
        default:
            if (proxy_type == old_proxy_type)
                break;
            proxy_changed = true;
            mgr->setProxy(SettingsManager::NoneProxy);
            break;
    }

    return proxy_changed;
}
