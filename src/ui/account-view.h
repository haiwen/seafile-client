#include <vector>
#include <QWidget>

#include "account-mgr.h"
#include "ui_account-view.h"

class AccountItem;
class QVBoxLayout;

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
    QVBoxLayout *getLayout();
    std::vector<AccountItem*> accounts_list_;
};
