#ifndef SEAFILE_CLIENT_UI_ACCOUNT_VIEW_H
#define SEAFILE_CLIENT_UI_ACCOUNT_VIEW_H

#include <QWidget>
#include "ui_account-view.h"

class Account;
class QAction;
class QMenu;

/*
 * The account information area, right below the header
 */
class AccountView : public QWidget,
                    public Ui::AccountView
{
    Q_OBJECT
public:
    AccountView(QWidget *parent=0);

public slots:
    void onAccountsChanged();
    void showAddAccountDialog();
    void deleteAccount();
    void onAccountItemClicked();

private:
    Q_DISABLE_COPY(AccountView)

    QAction *makeAccountAction(const Account& account);
    void updateAccountInfoDisplay();

    // Account operations
    QAction *add_account_action_;
    QAction *delete_account_action_;
    QMenu *account_menu_;
};

#endif // SEAFILE_CLIENT_UI_ACCOUNT_VIEW_H
