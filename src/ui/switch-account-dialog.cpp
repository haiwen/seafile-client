#include <vector>
#include <QIcon>
#include <QListWidgetItem>

#include "account-mgr.h"
#include "seafile-applet.h"
#include "switch-account-dialog.h"

SwitchAccountDialog::SwitchAccountDialog(QWidget *parent) : QDialog(parent)
{
    setupUi(this);
    setWindowTitle(tr("Choose an account"));

    initAccountList();

    connect(mSubmitBtn, SIGNAL(clicked()), this, SLOT(submit()));
}

void SwitchAccountDialog::initAccountList()
{
    accounts_ = seafApplet->accountManager()->loadAccounts();
    int i, n = accounts_.size();
    for (i = 0; i < n; i++) {
        Account account = accounts_[i];
        QIcon icon = QIcon(":/images/account.svg");
        QUrl url = account.serverUrl;
        QString text = account.username + "\t" + url.host();
        if (url.port() > 0) {
            text += QString().sprintf(":%d", url.port());
        }
        if (url.path().length() > 0) {
            text += url.path();
        }

        QListWidgetItem *item = new QListWidgetItem(icon, text);
        mAccountsList->addItem(item);
    }

    if (!accounts_.empty()) {
        mAccountsList->setCurrentRow(0);
    }

    mAccountsList->setSelectionMode(QAbstractItemView::SingleSelection);
}

void SwitchAccountDialog::submit()
{
    if (accounts_.empty()) {
        return;
    }
    int row = mAccountsList->currentRow();
    emit accountSelected(accounts_[row]);
    return;
}
