#ifndef SEAFILE_CLIENT_UI_ACCOUNT_VIEW_H
#define SEAFILE_CLIENT_UI_ACCOUNT_VIEW_H

#include <QWidget>

#include "utils/singleton.h"
#include "ui_account-view.h"

class Account;
class QAction;
class QMenu;
class ApiError;
class QLabel;

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
    void onAccountChanged();
    void showAddAccountDialog();
    void deleteAccount();
    void editAccountSettings();
    void onAccountItemClicked();
    void updateAccountInfoDisplay();

private slots:
    void updateAvatar();
    void toggleAccount();
    void visitServerInBrowser(const QString& link);
    void slotShowSettingsDialog();

private:
    Q_DISABLE_COPY(AccountView)

    QAction *makeAccountAction(const Account& account);
    void setupSortingMenu();
    bool eventFilter(QObject *obj, QEvent *event);

    // Account operations
    QAction *add_account_action_;
    QAction *account_settings_action_;
    QMenu *account_menu_;
    QMenu *sorting_menu_;

signals:
    void refresh();
    void sortOrderUpdated();
};

#endif // SEAFILE_CLIENT_UI_ACCOUNT_VIEW_H
