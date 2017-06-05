#ifndef SEAFILE_CLIENT_FILE_BROWSER_ADVANCEDSHAREDLINK_DIALOG_H
#define SEAFILE_CLIENT_FILE_BROWSER_ADVANCEDSHAREDLINK_DIALOG_H
#include <QDialog>
#include <QSpinBox>
#include <QGroupBox>

#include "account.h"

struct SharedLinkInfo;
class CreateShareLinkRequest;
class Account;
class QLineEdit;

class AdvancedSharedLinkDialog : public QDialog
{
    Q_OBJECT
public:
    // AdvancedSharedLinkDialog(QWidget *parent);
    AdvancedSharedLinkDialog(QWidget *parent,
                             const Account &account,
                             const QString &repo_id,
                             const QString &path);

private slots:
    void onCopyText();
    void onOkBtnClicked();
    void onPreviousSharedLinkExist(const SharedLinkInfo& info);
    void generateAdvancedSharedLinkSuccess(const SharedLinkInfo& shared_link_info);

private:
    void checkPreviousSharedLink();

    Account account_;
    QString repo_id_;
    QString path_;

    QString password_;
    QString password_again_;
    quint64 valid_days_;

    QGroupBox *pwdGroupBox_;
    QGroupBox *expiredDateGroupBox_;
    QLineEdit *pwdEdit_;
    QLineEdit *pwdEdit2_;
    QLineEdit *editor_;
    QSpinBox *expiredDateSpinBox_;

    CreateShareLinkRequest *advanced_share_req_;
};

#endif
