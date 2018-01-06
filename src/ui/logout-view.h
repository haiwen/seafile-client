#ifndef SEAFILE_CLIENT_LOGOUT_VIEW_H_
#define SEAFILE_CLIENT_LOGOUT_VIEW_H_

#include <QWidget>

class QShowEvent;
class QLabel;
class Account;

class LogoutView : public QWidget {
    Q_OBJECT
public:
    LogoutView(QWidget *parent=0);
    void setQssStyleForTab();

private slots:
    void onAccountChanged();
    void reloginCurrentAccount();

private:
    Q_DISABLE_COPY(LogoutView)
    QLabel *label_;
};

#endif // SEAFILE_CLIENT_LOGOUT_VIEW_H_
