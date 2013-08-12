#include "account-mgr.h"

AccountManager::AccountManager()
{
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
