#include "seafile-applet.h"
#include "ssl-confirm-dialog.h"

SslConfirmDialog::SslConfirmDialog(const QUrl& url,
                                   const QString fingerprint,
                                   const QString prev_fingerprint,
                                   QWidget *parent)
    : QDialog(parent),
      url_(url)
{
    setupUi(this);

    setWindowTitle(tr("Untrusted Connection"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    QString hint = tr("%1 uses an invalid security certificate. The connection may be insecure. Do you want to continue?").arg(url_.host());

    hint += "\n";
    hint += tr("Current RSA key fingerprint is %1").arg(fingerprint);
    if (prev_fingerprint != "") {
        hint += "\n";
        hint += tr("Previous RSA key fingerprint is %1").arg(prev_fingerprint);
    }

    mHint->setText(hint);

    connect(mYesBtn, SIGNAL(clicked()), this, SLOT(accept()));
    connect(mNoBtn, SIGNAL(clicked()), this, SLOT(reject()));
}

bool
SslConfirmDialog::rememberChoice() const
{
    return mRememberChoiceCheckBox->checkState() == Qt::Checked;
}
