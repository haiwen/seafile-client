#include <QtGui>

#include "QtAwesome.h"
#include "server-repos-view.h"
#include "cloud-view.h"

CloudView::CloudView(QWidget *parent) : QWidget(parent)
{
    setupUi(this);

    server_repos_view_ = new ServerReposView;
    QVBoxLayout *layout = (QVBoxLayout *)(this->layout());
    layout->addWidget(server_repos_view_);

    createAccoutOpMenu();
    prepareAccountOpButton();
}

void CloudView::prepareAccountOpButton()
{
    mAccountOpButton->setText(QChar(icon_user));
    mAccountOpButton->setFont(awesome->font(24));

    // mAccountOpButton->setContextMenuPolicy(Qt::CustomContextMenu);
    // connect(mAccountOpButton, SIGNAL(customContextMenuRequested(const QPoint &)),
    //         this, SLOT(showAccoutOpMenu(const QPoint &)));

    mAccountOpButton->setMenu(accout_op_menu_);
    connect(mAccountOpButton, SIGNAL(clicked()), mAccountOpButton, SLOT(showMenu()));
}

void CloudView::createAccoutOpMenu()
{
    accout_op_menu_ = new QMenu;

    switch_account_action_ = new QAction(tr("Switch account"), this);
    add_account_action_ = new QAction(tr("Add an account"), this);
    delete_account_action_ = new QAction(tr("Delete this account"), this);

    connect(switch_account_action_, SIGNAL(triggered()), this, SLOT(switchAccount()));
    connect(add_account_action_, SIGNAL(triggered()), this, SLOT(addAccount()));
    connect(delete_account_action_, SIGNAL(triggered()), this, SLOT(deleteAccount()));

    accout_op_menu_->addAction(switch_account_action_);
    accout_op_menu_->addAction(add_account_action_);
    accout_op_menu_->addAction(delete_account_action_);
}

void CloudView::switchAccount()
{
}

void CloudView::addAccount()
{
}

void CloudView::deleteAccount()
{
}
