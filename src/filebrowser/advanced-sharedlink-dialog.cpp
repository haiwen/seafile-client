#include "advanced-sharedlink-dialog.h"

#include <QtGlobal>
#include <QtWidgets>
#include <QHBoxLayout>
#include <QGridLayout>
#include "utils/utils-mac.h"
#include "file-browser-requests.h"
#include "seafile-applet.h"
#include "account.h"

AdvancedSharedLinkDialog::AdvancedSharedLinkDialog(QWidget *parent,
                                                   const Account &account,
                                                   const QString &repo_id,
                                                   const QString &path)
    :valid_days_(0)
{
    setWindowTitle(tr("Share Link"));
    setWindowIcon(QIcon(":/images/seafile.png"));
    setWindowFlags((windowFlags() & ~Qt::WindowContextHelpButtonHint) |
                   Qt::WindowStaysOnTopHint);
    QVBoxLayout *layout = new QVBoxLayout;

    QLabel *label = new QLabel(tr("Share link:"));
    layout->addWidget(label);
    layout->setSpacing(5);
    layout->setContentsMargins(9, 9, 9, 9);

    editor_ = new QLineEdit;
    layout->addWidget(editor_);
    editor_->setReadOnly(true);

    pwdGroupBox_ = new QGroupBox("Add password protection(at least 8 characters)");
    pwdGroupBox_->setCheckable(true);
    pwdGroupBox_->setChecked(true);
    QGridLayout *gridLayout = new QGridLayout();
    QLabel *pwdLabel = new QLabel(tr("password:"));
    pwdEdit_ = new QLineEdit;
    pwdEdit_->setEchoMode(QLineEdit::Password);
    QLabel *pwdLabel2 = new QLabel(tr("password again:"));
    pwdEdit2_ = new QLineEdit;
    pwdEdit2_->setEchoMode(QLineEdit::Password);
    gridLayout->addWidget(pwdLabel, 0, 0);
    gridLayout->addWidget(pwdEdit_, 0, 1);
    gridLayout->addWidget(pwdLabel2, 1, 0);
    gridLayout->addWidget(pwdEdit2_, 1, 1);
    pwdGroupBox_->setLayout(gridLayout);
    layout->addWidget(pwdGroupBox_);

    expiredDateGroupBox_ = new QGroupBox("Add auto expiration");
    expiredDateGroupBox_->setCheckable(true);
    expiredDateGroupBox_->setChecked(true);
    QHBoxLayout *expiredDateLayout = new QHBoxLayout();
    QLabel *expiredDateLabel = new QLabel(tr("Days:"));
    expiredDateSpinBox_ = new QSpinBox();
    expiredDateSpinBox_->setMinimum(1);
    expiredDateLayout->addWidget(expiredDateLabel);
    expiredDateLayout->addWidget(expiredDateSpinBox_);
    expiredDateGroupBox_->setLayout(expiredDateLayout);
    layout->addWidget(expiredDateGroupBox_);

    QHBoxLayout *hlayout = new QHBoxLayout;

    QWidget *spacer = new QWidget;
    spacer->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    hlayout->addWidget(spacer);

    QWidget *spacer2 = new QWidget;
    spacer2->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    hlayout->addWidget(spacer2);

    QPushButton *copy_to = new QPushButton(tr("Copy to clipboard"));
    hlayout->addWidget(copy_to);
    connect(copy_to, SIGNAL(clicked()), this, SLOT(onCopyText()));

    QPushButton *ok = new QPushButton(tr("OK"));
    hlayout->addWidget(ok);
    connect(ok, SIGNAL(clicked()), this, SLOT(onOkBtnClicked()));

    layout->addLayout(hlayout);

    setLayout(layout);

    setMinimumWidth(300);
    setMaximumWidth(400);

    advanced_share_req_ = new CreateShareLinkRequest(
        account, repo_id, path);

    connect(advanced_share_req_, SIGNAL(success(const SharedLinkInfo&)),
            this, SLOT(generateAdvancedSharedLinkSuccess(const SharedLinkInfo&)));
}

void AdvancedSharedLinkDialog::onCopyText()
{
// for mac, qt copys many minedatas beside public.utf8-plain-text
// e.g. public.vcard, which we don't want to use
#ifndef Q_OS_MAC
    QApplication::clipboard()->setText(editor_->text());
#else
    utils::mac::copyTextToPasteboard(editor_->text());
#endif
}

void AdvancedSharedLinkDialog::onOkBtnClicked()
{
    if (editor_->text().isEmpty() == false) {
        seafApplet->warningBox(tr("Advanced share link is already generated."), this);
        return;
    }
    if (expiredDateGroupBox_->isChecked()) {
        valid_days_ = expiredDateSpinBox_->value();
    }
    if (pwdGroupBox_->isChecked()) {
        password_ = pwdEdit_->text();
        password_again_ = pwdEdit2_->text();
        if (QString::compare(password_, password_again_) != 0) {
            seafApplet->warningBox(tr("passwords don't match."), this);
            pwdEdit_->clear();
            pwdEdit2_->clear();
            return;
        } else if (password_.length() < 8) {
            seafApplet->warningBox(tr("passwords at least 8 characters."), this);
            pwdEdit_->clear();
            pwdEdit2_->clear();
            return;
	}
    }

    advanced_share_req_->SetAdvancedShareParams(password_, valid_days_);
    advanced_share_req_->send();

    // emit generateAdvancedShareLink(password_, valid_days_);
}

void AdvancedSharedLinkDialog::generateAdvancedSharedLinkSuccess(const SharedLinkInfo& shared_link_info)
{
    editor_->setText(shared_link_info.link);
    editor_->selectAll();
}
