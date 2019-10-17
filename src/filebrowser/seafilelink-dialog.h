#ifndef SEAFILE_CLIENT_FILE_BROWSER_SEAFILELINK_DIALOG_H
#define SEAFILE_CLIENT_FILE_BROWSER_SEAFILELINK_DIALOG_H
#include <QDialog>

class QLineEdit;
class Account;
class SeafileLinkDialog : public QDialog
{
    Q_OBJECT
public:
    SeafileLinkDialog(const QString& smart_link, const QString& protocol_link, QWidget *parent);

private slots:
    void onCopyWebText();
    void onCopyProtocolText();

private:
    QString web_link_;
    const QString protocol_link_;
    QLineEdit *web_editor_;
    QLineEdit *protocol_editor_;
};

#endif
