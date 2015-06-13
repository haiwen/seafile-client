
#include "seafile-applet.h"
#include "account-mgr.h"
#include "api/api-error.h"
#include "api/requests.h"

#include "set-repo-password-dialog.h"

SetRepoPasswordDialog::SetRepoPasswordDialog(const ServerRepo& repo, QWidget *parent)
    : QDialog(parent),
      repo_(repo)
{
    setupUi(this);
    setWindowTitle(tr("Please provide the library password"));
    setWindowIcon(QIcon(":/images/seafile.png"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    request_ = 0;

    QString name = QString("<b>%1</b>").arg(repo.name);
    mHintText->setText(tr("Provide the password for library %1").arg(name));

    mErrorText->setVisible(false);

    connect(mOkBtn, SIGNAL(clicked()), this, SLOT(onOkBtnClicked()));
}

void SetRepoPasswordDialog::onOkBtnClicked()
{
    mErrorText->setVisible(false);

    password_ = mPassword->text().trimmed();

    if (password_.isEmpty()) {
        QString msg = tr("Please enter the password");
        seafApplet->warningBox(msg, this);
        return;
    }

    disableInputs();

    const Account& account = seafApplet->accountManager()->currentAccount();

    if (request_) {
        delete request_;
    }

    request_ = new SetRepoPasswordRequest(account, repo_.id, password_);
    connect(request_, SIGNAL(success()),
            this, SLOT(accept()));
    connect(request_, SIGNAL(failed(const ApiError&)),
            this, SLOT(requestFailed(const ApiError&)));

    request_->send();
}

void SetRepoPasswordDialog::requestFailed(const ApiError& error)
{
    QString msg;
    if (error.httpErrorCode() == 400) {
        msg = tr("Incorrect password");
    } else {
        msg = tr("Unknown error");
    }

    mErrorText->setVisible(true);
    mErrorText->setText(msg);

    enableInputs();
}

void SetRepoPasswordDialog::enableInputs()
{
    mOkBtn->setEnabled(true);
    mCancelBtn->setEnabled(true);
    mPassword->setEnabled(true);
}

void SetRepoPasswordDialog::disableInputs()
{
    mOkBtn->setEnabled(false);
    mCancelBtn->setEnabled(false);
    mPassword->setEnabled(false);
}
