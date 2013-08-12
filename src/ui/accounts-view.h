#include <vector>
#include <QWidget>

#include "account-mgr.h"
#include "ui_accounts-view.h"

class AccountItem;
class Account;


class AccountsView : public QWidget,
                     public Ui::AccountsView
{
    Q_OBJECT

public:
    AccountsView(QWidget *parent=0);

private slots:
    void showAddAccountDialog();
    void refreshAccounts();

private:
    bool hasAccount(const Account& account);

    std::vector<AccountItem*> accounts_list_;
    QWidget *accounts_list_view_;
};

