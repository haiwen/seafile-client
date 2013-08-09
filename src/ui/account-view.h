#include <vector>
#include <QWidget>

#include "account-mgr.h"
#include "ui_account-view.h"

class AccountItem;

class AccountView : public QWidget,
                    public Ui::AccountView
{
    Q_OBJECT

public:
    AccountView(QWidget *parent=0);

private slots:
    void showAddAccountDialog();
    void refreshAccounts();

private:
    std::vector<AccountItem*> accounts_list_;
    QWidget *accounts_list_view_;
};
