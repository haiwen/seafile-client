#include "seafile-applet.h"
#include "ssl-confirm-dialog.h"

#include "utils/utils.h"

namespace {
const int kMaximumHeight = 450;
const int kDetailTextEditHeight = 200;
}

SslConfirmDialog::SslConfirmDialog(const QUrl& url,
                                   const QSslCertificate& cert,
                                   const QSslCertificate& prev_cert,
                                   QWidget *parent)
    : QDialog(parent),
      url_(url)
{
    setupUi(this);

    setWindowTitle(tr("Untrusted Connection"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    QString hint = tr("%1 uses an invalid security certificate. The connection may be insecure. Do you want to continue?").arg(url_.host());

    QString fingerprint = dumpCertificateFingerprint(cert, QCryptographicHash::Sha1);
    QString prev_fingerprint = dumpCertificateFingerprint(prev_cert, QCryptographicHash::Sha1);

    hint += "\n\n";
    hint += tr("Current RSA key SHA1 fingerprint is %1").arg(fingerprint);
    if (prev_fingerprint != "") {
        hint += "\n";
        hint += tr("Previous RSA key SHA1 fingerprint is %1").arg(prev_fingerprint);
    }

    mHint->setText(hint);

    mDetailTextEdit->setText(tr("Current Certificate:%1\nPrevious Certificate:%2\n")
            .arg(dumpCertificate(cert))
            .arg(dumpCertificate(prev_cert)));
    setMinimumHeight(kMaximumHeight - kDetailTextEditHeight);
    setMaximumHeight(kMaximumHeight - kDetailTextEditHeight);
    mDetailTextEdit->setVisible(false);

    connect(mYesBtn, SIGNAL(clicked()), this, SLOT(accept()));
    connect(mNoBtn, SIGNAL(clicked()), this, SLOT(reject()));

    connect(mShowDetailBtn, SIGNAL(clicked()), this, SLOT(onShowMoreDetails()));
}

bool SslConfirmDialog::rememberChoice() const
{
    return mRememberChoiceCheckBox->checkState() == Qt::Checked;
}

void SslConfirmDialog::onShowMoreDetails()
{
    bool show = mDetailTextEdit->isVisible();
    if (show) {
        setMinimumHeight(kMaximumHeight - kDetailTextEditHeight);
        setMaximumHeight(kMaximumHeight - kDetailTextEditHeight);
    } else {
        setMinimumHeight(kMaximumHeight);
        setMaximumHeight(kMaximumHeight);
    }
    mDetailTextEdit->setVisible(!show);
}
