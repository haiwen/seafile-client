#include "seafile-applet.h"
#include "ssl-confirm-dialog.h"

SslConfirmDialog::SslConfirmDialog(const QUrl& url,
                                   QWidget *parent)
    : QDialog(parent),
      url_(url)
{
    setupUi(this);

    setWindowTitle(tr("Untrusted Connection"));

    mIcon->setPixmap(QPixmap(":/images/sync/exclamation.png"));

    QString hint = tr("%1 uses an invalid security certificate. Possible reasons are:").arg(url_.host());
    mHint->setText(hint);

    QString error_text;
    error_text += "<ul>";

    error_text += "<li>";
    error_text += tr("The server has changed its certificate");
    error_text += "</li>";

    error_text += "<li>";
    error_text += tr("You are under security attack");
    error_text += "</li>";

    error_text += "</ul>";

    mErrorDetail->setText(error_text);

    QString question = tr("Do you still wish to continue?");
    mQuestion->setText(question);

    connect(mYesBtn, SIGNAL(clicked()), this, SLOT(accept()));
    connect(mNoBtn, SIGNAL(clicked()), this, SLOT(reject()));
}

bool
SslConfirmDialog::rememberChoice() const
{
    return mRememberChoiceCheckBox->checkState() == Qt::Checked;
}
