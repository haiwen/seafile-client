#include <QtGui>

#include "get-account-repos-dialog.h"
#include "seaf-repo.h"

GetAccountReposDialog::GetAccountReposDialog(const Account& account, QWidget *parent)
    : QDialog(parent),
      account_(account)
{
    setupUi(this);

    // SeafileApiClient *sc = SeafileApiClient::instance();

    // sc->getAccountRepos(account);

    // connect(sc, SIGNAL(getAccountReposSuccess(const std::vector<SeafRepo>&)),
    //         this, SLOT(onRequestSuccess(const std::vector<SeafRepo>&)));

    // connect(sc, SIGNAL(getAccountReposFailed()), this, SLOT(onRequestFailed()));
}

void GetAccountReposDialog::onRequestSuccess(const std::vector<SeafRepo>& repos)
{
}

void GetAccountReposDialog::onRequestFailed()
{
}
