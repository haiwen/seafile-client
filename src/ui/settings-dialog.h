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

    void autoStartChanged(int state);
    void hideDockIconChanged(int state);
    void notifyChanged(int state);
    void downloadChanged(int value);
    void uploadChanged(int value);
    void closeEvent(QCloseEvent *event);
    void showEvent(QShowEvent *event);
    void updateSettings();
    void onOkBtnClicked();

    void proxyRequirePasswordChanged(int state);
    void showHideControlsBasedOnCurrentProxyType(int state);

private:
    bool updateProxySettings();
    bool validateProxyInputs();

    Q_DISABLE_COPY(SettingsDialog);
};
