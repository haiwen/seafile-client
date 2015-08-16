#include "seafilelink-dialog.h"

#include <QtGlobal>
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QtWidgets>
#else
#include <QtGui>
#endif
#include "QtAwesome.h"
#include "utils/utils.h"
#include "utils/utils-mac.h"
#include "open-local-helper.h"

SeafileLinkDialog::SeafileLinkDialog(const QString& repo_id, const Account& account, const QString& path, QWidget *parent)
    : web_link_(OpenLocalHelper::instance()->generateLocalFileWebUrl(repo_id, account, path).toEncoded())
    , protocol_link_(OpenLocalHelper::instance()->generateLocalFileSeafileUrl(repo_id, account, path).toEncoded())
{
    setWindowTitle(tr("%1 Internal Link").arg(getBrand()));
    setWindowIcon(QIcon(":/images/seafile.png"));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setSpacing(5);
    layout->setContentsMargins(9, 9, 9, 9);

    QString copy_to_str = tr("Copy to clipboard");
    QIcon copy_to_icon = awesome->icon(icon_copy);

    //
    // create web link related
    //
    QLabel *web_label = new QLabel(tr("%1 Web Link:").arg(getBrand()));
    layout->addWidget(web_label);
    QHBoxLayout *web_layout = new QHBoxLayout;

    web_editor_ = new QLineEdit;
    web_editor_->setText(web_link_);
    web_editor_->selectAll();
    web_editor_->setReadOnly(true);
    web_editor_->setCursorPosition(0);

    web_layout->addWidget(web_editor_);

    QPushButton *web_copy_to = new QPushButton;
    web_copy_to->setIcon(copy_to_icon);
    web_copy_to->setToolTip(copy_to_str);
    web_layout->addWidget(web_copy_to);
    connect(web_copy_to, SIGNAL(clicked()), this, SLOT(onCopyWebText()));
    layout->addLayout(web_layout);

    //
    // create seafile-protocol link related
    //
    QLabel *protocol_label = new QLabel(tr("%1 Protocol Link:").arg(getBrand()));
    layout->addWidget(protocol_label);
    QHBoxLayout *protocol_layout = new QHBoxLayout;

    protocol_editor_ = new QLineEdit;
    protocol_editor_->setText(protocol_link_);
    protocol_editor_->selectAll();
    protocol_editor_->setReadOnly(true);
    protocol_editor_->setCursorPosition(0);

    protocol_layout->addWidget(protocol_editor_);

    QPushButton *protocol_copy_to = new QPushButton;
    protocol_copy_to->setIcon(copy_to_icon);
    protocol_copy_to->setToolTip(copy_to_str);
    protocol_layout->addWidget(protocol_copy_to);
    connect(protocol_copy_to, SIGNAL(clicked()), this, SLOT(onCopyProtocolText()));
    layout->addLayout(protocol_layout);

    QHBoxLayout *hlayout = new QHBoxLayout;

    QWidget *spacer = new QWidget;
    spacer->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    hlayout->addWidget(spacer);

    QWidget *spacer2 = new QWidget;
    spacer2->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    hlayout->addWidget(spacer2);

    QWidget *spacer3 = new QWidget;
    spacer3->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    hlayout->addWidget(spacer3);

    QPushButton *ok = new QPushButton(tr("OK"));
    hlayout->addWidget(ok);
    connect(ok, SIGNAL(clicked()), this, SLOT(accept()));
    ok->setFocus();

    layout->addLayout(hlayout);

    setLayout(layout);

    setMinimumWidth(450);
}

void SeafileLinkDialog::onCopyWebText()
{
// for mac, qt copys many minedatas beside public.utf8-plain-text
// e.g. public.vcard, which we don't want to use
#ifndef Q_OS_MAC
    QApplication::clipboard()->setText(web_link_);
#else
    utils::mac::copyTextToPasteboard(web_link_);
#endif
}

void SeafileLinkDialog::onCopyProtocolText()
{
// for mac, qt copys many minedatas beside public.utf8-plain-text
// e.g. public.vcard, which we don't want to use
#ifndef Q_OS_MAC
    QApplication::clipboard()->setText(protocol_link_);
#else
    utils::mac::copyTextToPasteboard(protocol_link_);
#endif
}
