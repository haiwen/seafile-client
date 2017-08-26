#ifndef SEAFILE_CLIENT_UI_USER_NAME_COMPLETER_H
#define SEAFILE_CLIENT_UI_USER_NAME_COMPLETER_H

#include <QObject>

class QLineEdit;
class QTimer;
class QTreeWidget;

struct SeafileUser;
class ApiError;

#include "account.h"
#include "api/contact-share-info.h"
#include "api/requests.h"

class SeafileUserNameCompleter : public QObject
{
    Q_OBJECT

public:
    SeafileUserNameCompleter(const Account& account, QLineEdit *parent = 0);
    ~SeafileUserNameCompleter();
    bool eventFilter(QObject *obj, QEvent *ev) Q_DECL_OVERRIDE;

    const SeafileUser& currentSelectedUser() const;

private slots:
    void onSearchUsersSuccess(const QList<SeafileUser>& contacts);
    void onSearchUsersFailed(const ApiError& error);
    void doneCompletion();
    void preventSuggest();
    void autoSuggest();
    void showCompletion(const QList<SeafileUser>& users, const QString& pattern);
    void onAvatarUpdated(const QString& email, const QImage& avatar);

private:
    Account account_;

    QLineEdit *editor_;
    QTreeWidget *popup_;
    QTimer *timer_;

    QSet<QString> in_progress_search_requests_;

    struct CachedUsersEntry {
        QSet<SeafileUser> users;
        qint64 ts;
    };
    QHash<QString, CachedUsersEntry> cached_completion_users_by_pattern_;

    SeafileUser current_selected_user_;
};

#endif // SEAFILE_CLIENT_UI_USER_NAME_COMPLETER_H
