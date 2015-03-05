#include <QDialog>
#include "ui_settings-dialog.h"

#include <QUrl>
#include <QString>


class SettingsDialog : public QDialog,
                       public Ui::SettingsDialog
{
    Q_OBJECT
public:
    SettingsDialog(QWidget *parent=0);

private slots:
    void reject();

    void autoStartChanged(int state);
    void hideDockIconChanged(int state);
    void notifyChanged(int state);
    void downloadChanged(int value);
    void uploadChanged(int value);
    void closeEvent(QCloseEvent *event);
    void showEvent(QShowEvent *event);
    void updateSettings();

private:
    void readSettings();
    Q_DISABLE_COPY(SettingsDialog);
};
