#include <QtGlobal>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QtWidgets>
#else
#include <QtGui>
#endif
#include "seafile-applet.h"
#include "account-mgr.h"

#include "logout-view.h"

LogoutView::LogoutView(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("LogoutView");

    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);

    label_ = new QLabel;
    label_->setAlignment(Qt::AlignCenter);

    layout->addWidget(label_);
    layout->setContentsMargins(0, 0, 0, 0);

    connect(seafApplet->accountManager(), SIGNAL(accountsChanged()),
            this, SLOT(onAccountChanged()));

    onAccountChanged();
}

void LogoutView::setQssStyleForTab()
{
    static const char *kLogoutViewQss = "border: 0; margin: 0;"
                              "border-top: 1px solid #DCDCDE;"
                              "background-color: #F5F5F7;";

    setStyleSheet(kLogoutViewQss);
}

void LogoutView::onAccountChanged()
{
    // disconnect current signal
    disconnect(label_, SIGNAL(linkActivated(const QString&)), 0, 0);

    QString link = QString("<a style=\"color:#777\" href=\"#\">%1</a>");

    if (seafApplet->accountManager()->hasAccount()) {
        connect(label_, SIGNAL(linkActivated(const QString&)),
                this, SLOT(reloginCurrentAccount()));
        label_->setText(tr("You are logout. Please ") + link.arg(tr("login")));
    } else {
        connect(label_, SIGNAL(linkActivated(const QString&)),
                seafApplet->accountManager(), SIGNAL(requireAddAccount()));
        label_->setText(link.arg(tr("Add an account")));
    }
}

void LogoutView::reloginCurrentAccount()
{
    Account account = seafApplet->accountManager()->accounts().front();
    seafApplet->accountManager()->reloginAccount(account);
}
