#include "sharedlink-dialog.h"

#include <QtGlobal>
#include <QtWidgets>
#include "utils/utils-mac.h"
#include "account.h"
#include "account-mgr.h"
#include "api/api-error.h"
#include "seafile-applet.h"
#include "filebrowser/file-browser-requests.h"

SharedLinkDialog::SharedLinkDialog(const QString& link, const QString &repo_id,
                                   const QString &path_in_repo,
                                   QWidget *parent)
  : text_(link), repo_id_(repo_id),
    path_in_repo_(path_in_repo)
{
    setWindowTitle(tr("Share Link"));
    setWindowIcon(QIcon(":/images/seafile.png"));
    setWindowFlags((windowFlags() & ~Qt::WindowContextHelpButtonHint) |
                   Qt::WindowStaysOnTopHint);
    QVBoxLayout *layout = new QVBoxLayout;

    QLabel *password_label = new QLabel(tr("Password"));
    layout->addWidget(password_label);

    QHBoxLayout *passwd_hlayout = new QHBoxLayout;
    QCheckBox *show_password = new QCheckBox(tr("Show password"), this);
    connect(show_password, &QCheckBox::stateChanged,
            this, &SharedLinkDialog::slotShowPasswordCheckBoxClicked);
    passwd_hlayout->addWidget(show_password);

    password_editor_ = new QLineEdit;
    passwd_hlayout->addWidget(password_editor_);
    password_editor_->setEchoMode(QLineEdit::Password);
    layout->addLayout(passwd_hlayout);

    QLabel *expire_days_label = new QLabel(tr("Expire days"));
    layout->addWidget(expire_days_label);

    expire_days_editor_ = new QLineEdit;
    QIntValidator* intValidator = new QIntValidator;
    expire_days_editor_->setValidator(intValidator);
    layout->addWidget(expire_days_editor_);

    QLabel *label = new QLabel(tr("Share link:"));
    layout->addWidget(label);
    layout->setSpacing(5);
    layout->setContentsMargins(9, 9, 9, 9);

    editor_ = new QLineEdit;
    editor_->setText(text_);
    editor_->selectAll();
    editor_->setReadOnly(true);
    layout->addWidget(editor_);

    QHBoxLayout *hlayout = new QHBoxLayout;

    QCheckBox *is_download_checked = new QCheckBox(tr("Direct Download"));
    connect(is_download_checked, SIGNAL(stateChanged(int)),
            this, SLOT(onDownloadStateChanged(int)));
    hlayout->addWidget(is_download_checked);

    QWidget *spacer = new QWidget;
    spacer->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    hlayout->addWidget(spacer);

    QWidget *spacer2 = new QWidget;
    spacer2->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    hlayout->addWidget(spacer2);

    QPushButton *copy_to = new QPushButton(tr("Copy to clipboard"));
    hlayout->addWidget(copy_to);
    connect(copy_to, SIGNAL(clicked()), this, SLOT(onCopyText()));

    generate_link_pushbutton_ = new QPushButton(tr("Generate link"));
    hlayout->addWidget(generate_link_pushbutton_);
    connect(generate_link_pushbutton_, SIGNAL(clicked()), this, SLOT(slotGenSharedLink()));

    layout->addLayout(hlayout);

    setLayout(layout);

    if (!link.isEmpty()) {
        show_password->hide();
        password_label->hide();
        password_editor_->hide();
        expire_days_label->hide();
        expire_days_editor_->hide();
        generate_link_pushbutton_->hide();
    } else {
        is_download_checked->hide();
    }

    setMinimumWidth(300);
    setMaximumWidth(400);
}

void SharedLinkDialog::onCopyText()
{
// for mac, qt copys many minedatas beside public.utf8-plain-text
// e.g. public.vcard, which we don't want to use
#ifndef Q_OS_MAC
    QApplication::clipboard()->setText(editor_->text());
#else
    utils::mac::copyTextToPasteboard(editor_->text());
#endif
}

void SharedLinkDialog::onDownloadStateChanged(int state)
{
    if (state == Qt::Checked)
        editor_->setText(text_ + "?dl=1");
    else
        editor_->setText(text_);
}

void SharedLinkDialog::slotGenSharedLink()
{
    const Account account = seafApplet->accountManager()->currentAccount();
    if (!account.isValid()) {
        return;
    }

    QString password = password_editor_->text();
    QString expire_days = expire_days_editor_->text();

    CreateSharedLinkRequest *req = new CreateSharedLinkRequest(account, repo_id_, path_in_repo_, password, expire_days);

    connect(req, SIGNAL(success(const QString&)),
            this, SLOT(onCreateSharedLinkSuccess(const QString&)));
    connect(req, SIGNAL(failed(const ApiError&)),
            this, SLOT(onCreateSharedLinkFailed(const ApiError&)));
    req->send();
}

void SharedLinkDialog::onCreateSharedLinkSuccess(const QString& link)
{
    text_ = link;
    editor_->setText(text_);
}

void SharedLinkDialog::onCreateSharedLinkFailed(const ApiError& error)
{
    CreateSharedLinkRequest *req = qobject_cast<CreateSharedLinkRequest*>(sender());

    if (error.type() == ApiError::HTTP_ERROR && error.httpErrorCode() == 400) {
        seafApplet->warningBox(tr("Failed to generate share link: %1").arg(req->errorMsg()));
        return;
    }

    seafApplet->warningBox(tr("Failed to generate share link: %1").arg(error.toString()));
}

void SharedLinkDialog::slotShowPasswordCheckBoxClicked(int state)
{
    if (state == Qt::Checked) {
        password_editor_ -> setEchoMode(QLineEdit::Normal);
        return;
    }
    password_editor_ -> setEchoMode(QLineEdit::Password);
}
