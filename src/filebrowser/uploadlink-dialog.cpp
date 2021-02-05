#include "uploadlink-dialog.h"

#include <QtWidgets>
#include "utils/utils-mac.h"


UploadLinkDialog::UploadLinkDialog(const QString &text, QWidget *parent)
  : text_(text)
{
    setWindowTitle(tr("Upload Link"));
    setWindowIcon(QIcon(":/images/seafile.png"));
    setWindowFlags((windowFlags() & ~Qt::WindowContextHelpButtonHint) |
                   Qt::WindowStaysOnTopHint);
    QVBoxLayout *layout = new QVBoxLayout;

    QLabel *label = new QLabel(tr("Upload link:"));
    layout->addWidget(label);
    layout->setSpacing(5);
    layout->setContentsMargins(9, 9, 9, 9);

    editor_ = new QLineEdit;
    editor_->setText(text_);
    editor_->selectAll();
    editor_->setReadOnly(true);
    layout->addWidget(editor_);

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
    connect(ok, SIGNAL(clicked()), this, SLOT(accept()));

    layout->addLayout(hlayout);

    setLayout(layout);

    setMinimumWidth(300);
    setMaximumWidth(400);
}

void UploadLinkDialog::onCopyText()
{
// for mac, qt copys many minedatas beside public.utf8-plain-text
// e.g. public.vcard, which we don't want to use
#ifndef Q_OS_MAC
    QApplication::clipboard()->setText(editor_->text());
#else
    utils::mac::copyTextToPasteboard(editor_->text());
#endif
}

