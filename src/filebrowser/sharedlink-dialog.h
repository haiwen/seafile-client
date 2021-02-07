#ifndef SEAFILE_CLIENT_FILE_BROWSER_SHAREDLINK_DIALOG_H
#define SEAFILE_CLIENT_FILE_BROWSER_SHAREDLINK_DIALOG_H
#include <QDialog>

class QLineEdit;
class SharedLinkDialog : public QDialog
{
    Q_OBJECT
public:
    SharedLinkDialog(const QString &link,
                     const QString &repo_id,
                     const QString &path_in_repo,
                     QWidget *parent);

private slots:
    void onCopyText();
    void onDownloadStateChanged(int state);
    void slotGenSharedLink();
    void slotGetSharedLink(const QString& link);
    void slotPasswordEditTextChanged(const QString& text);
    void slotShowPasswordCheckBoxClicked(int state);

private:
    QString text_;
    const QString repo_id_;
    const QString path_in_repo_;

    QLineEdit *editor_;
    QLineEdit *password_editor_;
    QLineEdit *expire_days_editor_;
    QPushButton *generate_link_pushbutton_;
};

#endif
