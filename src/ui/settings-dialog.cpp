#include <QtGui>
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

    #ifdef Q_WS_MAC
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

    if (isCheckLatestVersionEnabled()) {
        bool enabled = mCheckLatestVersionBox->checkState() == Qt::Checked;
        mgr->setCheckLatestVersionEnabled(enabled);
    }

    I18NHelper::getInstance()->setPreferredLanguage(mLanguageComboBox->currentIndex());
}

void SettingsDialog::closeEvent(QCloseEvent *event)
{
    updateSettings();

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
#if !defined(Q_WS_WIN) && !defined(Q_WS_MAC)
    mAutoStartCheckBox->hide();
#endif

    // currently supports mac only
    state = mgr->hideDockIcon() ? Qt::Checked : Qt::Unchecked;
    mHideDockIconCheckBox->setCheckState(state);
#if !defined(Q_WS_MAC)
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
