#include "two-factor-dialog.h"
#include "seafile-applet.h"

TwoFactorDialog::TwoFactorDialog(QWidget *parent) :
    QDialog(parent)
{
    setupUi(this);
    mText->setText(tr("Enter the two factor authentication token"));
    setWindowTitle(tr("Two Factor Authentication"));

    connect(mSubmit, SIGNAL(clicked()), this, SLOT(doSubmit()));
}

QString TwoFactorDialog::getText()
{
    return mLineEdit->text();
}

bool TwoFactorDialog::rememberDeviceChecked()
{
    return mRememberDevice->isChecked();
}

void TwoFactorDialog::doSubmit()
{
    if (!mLineEdit->text().isEmpty()) {
        accept();
    } else {
        seafApplet->warningBox(tr("Please enter the two factor authentication token"));
    }
}
