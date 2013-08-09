#include "account-mgr.h"

AccountManager* AccountManager::singleton_ = NULL;

AccountManager* AccountManager::instance()
{
    if (singleton_ == 0) {
        singleton_ = new AccountManager();
    }

    return singleton_;
}

AccountManager::AccountManager()
{
    Account fake_account(QUrl("https://fake.seafile.com"), "fake@fake.com", "__fake__token__");
    accounts_.push_back(fake_account);
    accounts_.push_back(fake_account);
}

std::vector<Account> AccountManager::loadAccounts()
{
    // TODO: load accounts from database
    return accounts_;
}

int AccountManager::saveAccount(const Account& account)
{
    // TODO: save an account into database
    accounts_.push_back(account);
    return 0;
}
