#ifndef SEAFILE_CLIENT_FILE_BROWSER_SEAFILELINK_DIALOG_H
#define SEAFILE_CLIENT_FILE_BROWSER_SEAFILELINK_DIALOG_H
#include <QDialog>

class QLineEdit;
class Account;
class SeafileLinkDialog : public QDialog
{
    Q_OBJECT
public:
    SeafileLinkDialog(const QString& repo_id, const Account& account, const QString& path, QWidget *parent = NULL);

private slots:
    void onCopyWebText();
    void onCopyProtocolText();

private:
    const QString web_link_;
    const QString protocol_link_;
    QLineEdit *web_editor_;
    QLineEdit *protocol_editor_;
};

#endif
