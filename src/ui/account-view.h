#ifndef SEAFILE_CLIENT_UI_ACCOUNT_VIEW_H
#define SEAFILE_CLIENT_UI_ACCOUNT_VIEW_H

#include <QWidget>
#include "ui_account-view.h"

class Account;
class QAction;
class QMenu;
class ApiError;

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

private slots:
    void updateAvatar();
    void logoutAccount();
    void reloginAccount(const Account &account);
    void onLogoutDeviceRequestSuccess();
    void onLogoutDeviceRequestFailed(const ApiError& error);
    void onGetRepoTokensSuccess();
    void onGetRepoTokensFailed(const ApiError& error);
    void visitServerInBrowser(const QString& link);
    void onGetLoginTokenSuccess(const QString& token);
    void onGetLoginTokenFailed(const ApiError& error);

private:
    Q_DISABLE_COPY(AccountView)

    QAction *makeAccountAction(const Account& account);
    void updateAccountInfoDisplay();
    bool eventFilter(QObject *obj, QEvent *event);
    void getRepoTokenWhenRelogin(const Account& account);

    // Account operations
    QAction *add_account_action_;
    QAction *logout_action_;
    QAction *account_settings_action_;
    QAction *delete_account_action_;
    QMenu *account_menu_;
};

#endif // SEAFILE_CLIENT_UI_ACCOUNT_VIEW_H
