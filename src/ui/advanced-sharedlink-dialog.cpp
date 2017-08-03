#include <QtGlobal>
#include <QtWidgets>
#include <QHBoxLayout>
#include <QGridLayout>

#include "utils/utils-mac.h"
#include "api/requests.h"

#include "seafile-applet.h"
#include "advanced-sharedlink-dialog.h"

AdvancedSharedLinkDialog::AdvancedSharedLinkDialog(QWidget *parent,
                                                   const SharedLinkRequestParams &params)
    : valid_days_(0)
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

    pwd_group_box_ = new QGroupBox("Add password protection(at least 8 characters)");
    pwd_group_box_->setCheckable(true);
    pwd_group_box_->setChecked(true);
    QGridLayout *grid_layout = new QGridLayout();
    QLabel *pwd_label = new QLabel(tr("password:"));
    pwd_edit_ = new QLineEdit;
    pwd_edit_->setEchoMode(QLineEdit::Password);
    QLabel *pwd_label2 = new QLabel(tr("password again:"));
    pwd_edit_again_ = new QLineEdit;
    pwd_edit_again_->setEchoMode(QLineEdit::Password);
    grid_layout->addWidget(pwd_label, 0, 0);
    grid_layout->addWidget(pwd_edit_, 0, 1);
    grid_layout->addWidget(pwd_label2, 1, 0);
    grid_layout->addWidget(pwd_edit_again_, 1, 1);
    pwd_group_box_->setLayout(grid_layout);
    layout->addWidget(pwd_group_box_);

    expired_date_group_box_ = new QGroupBox("Add auto expiration");
    expired_date_group_box_->setCheckable(true);
    expired_date_group_box_->setChecked(true);
    QHBoxLayout *expired_date_layout = new QHBoxLayout();
    QLabel *expired_date_label = new QLabel(tr("Days:"));
    expired_date_spin_box_ = new QSpinBox();
    expired_date_spin_box_->setMinimum(1);
    expired_date_layout->addWidget(expired_date_label);
    expired_date_layout->addWidget(expired_date_spin_box_);
    expired_date_group_box_->setLayout(expired_date_layout);
    layout->addWidget(expired_date_group_box_);

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

    advanced_share_req_ = new CreateShareLinkRequest(params);

    connect(advanced_share_req_, SIGNAL(success(const SharedLinkInfo&)),
            this, SLOT(onGenerateAdvancedSharedLinkSuccess(const SharedLinkInfo&)));
    connect(advanced_share_req_, SIGNAL(failed()),
            this, SLOT(onGenerateAdvancedSharedLinkFailed()));
}

AdvancedSharedLinkDialog::~AdvancedSharedLinkDialog()
{
    advanced_share_req_->deleteLater();
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
    if (!editor_->text().isEmpty()) {
        seafApplet->warningBox(tr("Advanced share link is already generated."), this);
        return;
    }
    if (expired_date_group_box_->isChecked()) {
        valid_days_ = expired_date_spin_box_->value();
    }
    if (pwd_group_box_->isChecked()) {
        password_ = pwd_edit_->text();
        password_again_ = pwd_edit_again_->text();
        if (QString::compare(password_, password_again_) != 0) {
            seafApplet->warningBox(tr("passwords don't match."), this);
            pwd_edit_->clear();
            pwd_edit_again_->clear();
            return;
        } else if (password_.length() < 8) {
            seafApplet->warningBox(tr("passwords at least 8 characters."), this);
            pwd_edit_->clear();
            pwd_edit_again_->clear();
            return;
	}
    }

    advanced_share_req_->SetAdvancedShareParams(password_, valid_days_);
    advanced_share_req_->send();
}

void AdvancedSharedLinkDialog::onGenerateAdvancedSharedLinkSuccess(const SharedLinkInfo& shared_link_info)
{
    editor_->setText(shared_link_info.link);
    editor_->selectAll();
}

void AdvancedSharedLinkDialog::onGenerateAdvancedSharedLinkFailed()
{
    seafApplet->warningBox(tr("Failed to generate the advanced shared link."), this);
}
