#include <QDialog>
#include <QUrl>
#include <QString>

#include "ui_create-repo.h"
#include "account.h"

class CreateRepoRequest;

class CreateRepoDialog : public QDialog,
                         public Ui::CreateRepoDialog
{
    Q_OBJECT
public:
    CreateRepoDialog(const Account& account, QWidget *parent=0);
    ~CreateRepoDialog();

private slots:
    void encryptedChanged(int state);
    void createAction();
    void chooseDirAction();
    void createSuccess(const QMap<QString, QString> &dict);
    void createFailed(int code);

    void cloneRepoRequestFinished(QString &repoId, bool result);


private:
    Q_DISABLE_COPY(CreateRepoDialog);
    bool validateInputs();

    QString path_;
    QString name_;
    QString desc_;
    QString passwd_;
    CreateRepoRequest *request_;
    Account account_;
};
