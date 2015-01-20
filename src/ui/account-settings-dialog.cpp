#include <QtGui>

#include "settings-mgr.h"
#include "account-mgr.h"
#include "seafile-applet.h"
#include "rpc/rpc-client.h"
#include "account-settings-dialog.h"

namespace {

} // namespace

AccountSettingsDialog::AccountSettingsDialog(const Account& account, QWidget *parent)
    : QDialog(parent),
      account_(account)
{
    setupUi(this);
    setWindowTitle(tr("Account Settings"));
    setWindowIcon(QIcon(":/images/seafile.png"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    mServerAddr->setText(account_.serverUrl.toString());
    mUsername->setText(account_.username);
    mUsername->setEnabled(false);

    #if defined(Q_OS_MAC)
    layout()->setContentsMargins(9, 9, 9, 9);
    layout()->setSpacing(6);
    formLayout->setSpacing(6);
    formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    formLayout->setLabelAlignment(Qt::AlignLeft);
    formLayout->setFormAlignment(Qt::AlignLeft);
    horizontalLayout->setSpacing(6);
    #endif

    connect(mOkBtn, SIGNAL(clicked()), this, SLOT(onSubmitBtnClicked()));

    // const QRect screen = QApplication::desktop()->screenGeometry();
    // move(screen.center() - this->rect().center());
}

void AccountSettingsDialog::showWarning(const QString& msg)
{
    seafApplet->warningBox(msg, this);
}

bool AccountSettingsDialog::validateInputs()
{
    QString server_addr = mServerAddr->text().trimmed();
    QUrl url;

    if (server_addr.size() == 0) {
        showWarning(tr("Please enter the server address"));
        return false;
    } else {
        if (!server_addr.startsWith("http://") && !server_addr.startsWith("https://")) {
            showWarning(tr("%1 is not a valid server address").arg(server_addr));
            return false;
        }

        url = QUrl(server_addr, QUrl::StrictMode);
        if (!url.isValid()) {
            showWarning(tr("%1 is not a valid server address").arg(server_addr));
            return false;
        }
    }

    return true;
}

void AccountSettingsDialog::onSubmitBtnClicked()
{
    if (!validateInputs()) {
        return;
    }

    QString url = mServerAddr->text().trimmed();
    if (url != account_.serverUrl.toString()) {
        Account new_account(account_);
        new_account.serverUrl = url;
        if (seafApplet->accountManager()->replaceAccount(account_,
            new_account) < 0) {
            showWarning(tr("Failed to save account information"));
            return;
        }
        QString error;
        if (seafApplet->rpcClient()->updateReposServerHost(account_.serverUrl.host(),
            new_account.serverUrl.host(), &error) < 0) {
            showWarning(tr("Failed to save the changes: %1").arg(error));
            return;
        }
    }

    seafApplet->messageBox(tr("Successfully updated current account information"), this);
    accept();
}
