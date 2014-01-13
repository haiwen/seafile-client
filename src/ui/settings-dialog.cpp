#include <QtGui>
#include <QDebug>

#include "account-mgr.h"
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

    // Since closeEvent() would not not called when accept() is called, we
    // need to handle it here.
    connect(this, SIGNAL(accepted()), this, SLOT(updateSettings()));

    if (!isCheckLatestVersionEnabled()) {
        mCheckLatestVersionBox->setVisible(false);
    }
}

void SettingsDialog::updateSettings()
{
    seafApplet->settingsManager()->setNotify(mNotifyCheckBox->checkState() == Qt::Checked);
    seafApplet->settingsManager()->setAutoStart(mAutoStartCheckBox->checkState() == Qt::Checked);
    seafApplet->settingsManager()->setMaxDownloadRatio(mDownloadSpinBox->value());
    seafApplet->settingsManager()->setMaxUploadRatio(mUploadSpinBox->value());
    seafApplet->settingsManager()->setHideMainWindowWhenStarted(mHideMainWinCheckBox->checkState() == Qt::Checked);

    if (isCheckLatestVersionEnabled()) {
        bool enabled = mCheckLatestVersionBox->checkState() == Qt::Checked;
        seafApplet->settingsManager()->setCheckLatestVersionEnabled(enabled);
    }
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

    state = seafApplet->settingsManager()->hideMainWindowWhenStarted() ? Qt::Checked : Qt::Unchecked;
    mHideMainWinCheckBox->setCheckState(state);

    state = seafApplet->settingsManager()->autoStart() ? Qt::Checked : Qt::Unchecked;
    mAutoStartCheckBox->setCheckState(state);
    state = seafApplet->settingsManager()->notify() ? Qt::Checked : Qt::Unchecked;
    mNotifyCheckBox->setCheckState(state);

    ratio = seafApplet->settingsManager()->maxDownloadRatio();
    mDownloadSpinBox->setValue(ratio);
    ratio = seafApplet->settingsManager()->maxUploadRatio();
    mUploadSpinBox->setValue(ratio);

    if (isCheckLatestVersionEnabled()) {
        state = seafApplet->settingsManager()->isCheckLatestVersionEnabled() ? Qt::Checked : Qt::Unchecked;
        mCheckLatestVersionBox->setCheckState(state);
    }

    QDialog::showEvent(event);
}


void SettingsDialog::autoStartChanged(int state)
{
    qDebug("%s :%d", __func__, state);
    bool autoStart = (mAutoStartCheckBox->checkState() == Qt::Checked);
    seafApplet->settingsManager()->setAutoStart(autoStart);
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
