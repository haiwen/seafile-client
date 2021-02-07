#ifndef SEAFILE_UPLOADLINK_DIALOG_H
#define SEAFILE_UPLOADLINK_DIALOG_H
#include <QDialog>

class QLineEdit;
class UploadLinkDialog : public QDialog
{
    Q_OBJECT
public:
    UploadLinkDialog(const QString &text, QWidget *parent);

private slots:
    void onCopyText();

private:
    const QString text_;
    QLineEdit *editor_;
};

#endif
