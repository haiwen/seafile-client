#include <QtGui>

#include "utils/gen_random_passwd.h"
#include "utils/password_strength.h"
#include "account-mgr.h"
#include "seafile-applet.h"
#include "configurator.h"
#include "api/requests.h"
#include "api/api-error.h"
#include "rpc/rpc-client.h"
#include "create-repo-dialog.h"

CreateRepoDialog::CreateRepoDialog(const Account& account,
                                   const QString& worktree,
                                   QWidget *parent)
    : QDialog(parent),
      request_(NULL),
      path_(worktree),
      account_(account)
{
    setupUi(this);
    setWindowTitle(tr("Create a library"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    mStatusText->setText("");

    mDirectory->setText(worktree);
    mName->setText(QDir(worktree).dirName());

    connect(mChooseDirBtn, SIGNAL(clicked()), this, SLOT(chooseDirAction()));
    connect(mOkBtn, SIGNAL(clicked()), this, SLOT(createAction()));
    connect(mGeneratePasswordBtn, SIGNAL(clicked()), this, SLOT(generatePassword()));
    connect(mEncryptedCheckBox, SIGNAL(clicked()), this, SLOT(passwordMode()));
    connect(mPassword, SIGNAL(textChanged(QString)), this, SLOT(calcPasswordStrength(QString)) );

    const QRect screen = QApplication::desktop()->screenGeometry();
    move(screen.center() - this->rect().center());
}

CreateRepoDialog::~CreateRepoDialog()
{
    if (request_)
        delete request_;
}

void CreateRepoDialog::chooseDirAction()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Please choose a directory"),
                                                    mDirectory->text(),
                                                    QFileDialog::ShowDirsOnly
                                                    | QFileDialog::DontResolveSymlinks);
    if (dir.isEmpty())
        return;
    mDirectory->setText(dir);
    if (mName->text().isEmpty()) {
        QDir d(dir);
        mName->setText(d.dirName());
    }
}

void CreateRepoDialog::setAllInputsEnabled(bool enabled)
{
    mOkBtn->setEnabled(enabled);
    mChooseDirBtn->setEnabled(enabled);
    mDirectory->setEnabled(enabled);
    mName->setEnabled(enabled);
    mDesc->setEnabled(enabled);
    mEncryptedCheckBox->setEnabled(enabled);

    bool password_enabled = (mEncryptedCheckBox->checkState() == Qt::Checked) && enabled;
    mPassword->setEnabled(password_enabled);
    mPasswordAgain->setEnabled(password_enabled);
}

void CreateRepoDialog::createAction()
{
    if (!validateInputs()) {
        return;
    }

    if( entropy <18 )
    {
        seafApplet->warningBox(tr("Password is too weak.\nMake sure you are using mixed-case,numbers and symbols as much as possible.\n\nOr generate one using the random password generator."), this);
        return;
    }

    mStatusText->setText(tr("Creating..."));

    setAllInputsEnabled(false);

    if (request_) {
        delete request_;
    }
    request_ = new CreateRepoRequest(account_, name_, desc_, passwd_);

    connect(request_, SIGNAL(success(const RepoDownloadInfo&)),
            this, SLOT(createSuccess(const RepoDownloadInfo&)));

    connect(request_, SIGNAL(failed(const ApiError&)),
            this, SLOT(createFailed(const ApiError&)));

    request_->send();
}

bool CreateRepoDialog::validateInputs()
{
    QString path;
    QString passwd;
    QString passwdAgain;
    bool encrypted;

    path = mDirectory->text();
    if (path.isEmpty()) {
        seafApplet->warningBox(tr("Please choose the directory to sync"), this);
        return false;
    }
    if (!QDir(path).exists()) {
        seafApplet->warningBox(tr("The folder %1 does not exist").arg(path), this);
        return false;
    }

    if (mName->text().isEmpty()) {
        seafApplet->warningBox(tr("Please enter the name"), this);
        return false;
    }

    if (mDesc->toPlainText().isEmpty()) {
        seafApplet->warningBox(tr("Please enter the description"), this);
        return false;
    }

    encrypted = (mEncryptedCheckBox->checkState() == Qt::Checked) ? true : false;
    if (encrypted) {
        if (mPassword->text().isEmpty() || mPasswordAgain->text().isEmpty()) {
            seafApplet->warningBox(tr("Please enter the password"), this);
            return false;
        }

        passwd = mPassword->text();
        passwdAgain = mPasswordAgain->text();
        if (passwd.length() < 8)
        {
            seafApplet->warningBox("Password must be at least 8 characters long", this);
            return false;
        }
        if (passwd != passwdAgain) {
            seafApplet->warningBox(tr("Passwords don't match"), this);
            return false;
        }
        passwd_ = passwd;
    } else {
        passwd_ = QString::null;
    }


    QString error;
    if (seafApplet->rpcClient()->checkPathForClone(path, &error) < 0) {
        if (error.isEmpty()) {
            error = tr("Unknown error");
        }
        seafApplet->warningBox(error, this);
        return false;
    }

    name_ = mName->text();
    desc_ = mDesc->toPlainText();
    path_ = mDirectory->text();
    return true;
}

void CreateRepoDialog::createSuccess(const RepoDownloadInfo& info)
{
    QString error;

    int ret = seafApplet->rpcClient()->cloneRepo(
        info.repo_id,
        info.repo_version,
        info.relay_id,
        info.repo_name,
        path_,
        info.token,
        passwd_,
        info.magic,
        info.relay_addr,
        info.relay_port,
        info.email,
        info.random_key,
        info.enc_version,
        &error);

    if (ret < 0) {
        QMessageBox::warning(this, getBrand(),
                             tr("Failed to add download task:\n %1").arg(error),
                             QMessageBox::Ok);
        setAllInputsEnabled(true);
    } else {
        done(QDialog::Accepted);
    }
}

void CreateRepoDialog::createFailed(const ApiError& error)
{
    mStatusText->setText("");

    QString msg = tr("Failed to create library on the server:\n%1").arg(error.toString());

    seafApplet->warningBox(msg, this);

    setAllInputsEnabled(true);
}

void CreateRepoDialog::generatePassword()
{
    const int key_length_bits = 256;
    char* generated_passwd = new char[4*(key_length_bits/4)/3 +1];

    // Make password field data visible to user
    mPassword->setEchoMode(QLineEdit::Normal);
    mPasswordAgain->setEchoMode(QLineEdit::Normal);

    // Generate 256 bit password
    int generated_password_length = gen_random_passwd(generated_passwd, key_length_bits/8);
    QString generated_passwdStr = QString(generated_passwd);
    generated_passwdStr.truncate(generated_password_length);
    mPassword->setText(generated_passwdStr);
    mPasswordAgain->setText(generated_passwdStr);
    mPassword->setReadOnly(true);
    mPassword->selectAll();
    mPasswordAgain->setReadOnly(true);
}

void CreateRepoDialog::passwordMode()
{
    mPassword->setEchoMode(QLineEdit::Password);
    mPassword->setReadOnly(false);
    mPasswordAgain->setEchoMode(QLineEdit::Password);
    mPasswordAgain->setReadOnly(false);
}

void CreateRepoDialog::calcPasswordStrength(QString password)
{
    QString strength_color;

    const int weak_entropy = 18;   
    const int medium_entropy = 22; 
    const int strong_entropy = 25; 
    
    bool weak_threshold=false;
    bool medium_threshold=false;    
    bool strong_threshold=false;

    if (password.isEmpty())
    {
        passwordStrengthLabel->setText("");    
        return;
    }
    
    if (password.length() < 8) {
        strength_color = "red";
        passwordStrengthLabel->setText("Password must be at least 8 characters long");
    } else {

        entropy = isStrongPassword(password.toStdString().c_str(), password.length(), weak_entropy);
        
        if (entropy >= strong_entropy) {
            strength_color = "green";      
            passwordStrengthLabel->setText("Strong password");
        } else {
            if (entropy > medium_entropy) {
                strength_color = "orange";     
                passwordStrengthLabel->setText("Medium password");
            } else {
                if (entropy >= weak_entropy) {
                    strength_color = "red";
                    passwordStrengthLabel->setText("Weak password");
                } else {
                    strength_color = "red";
                    passwordStrengthLabel->setText("Too weak password");
                }
            }
        }
    }
    passwordStrengthLabel->setStyleSheet(QString("QLabel { color : %1; }").arg(strength_color));
}
